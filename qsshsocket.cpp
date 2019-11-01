#include "qsshsocket.h"
#include <QFileInfo>
#include <QLineEdit>
#include <QInputDialog>
// if compiling in windows, add needed flags.
/*
#ifdef _WIN32
#include <io.h>

typedef int mode_t;

/// @Note If STRICT_UGO_PERMISSIONS is not defined, then setting Read for any
///       of User, Group, or Other will set Read for User and setting Write
///       will set Write for User.  Otherwise, Read and Write for Group and
///       Other are ignored.
///
/// @Note For the POSIX modes that do not have a Windows equivalent, the modes
///       defined here use the POSIX values left shifted 16 bits.

static const mode_t S_ISUID      = 0x08000000;           ///< does nothing
static const mode_t S_ISGID      = 0x04000000;           ///< does nothing
static const mode_t S_ISVTX      = 0x02000000;           ///< does nothing
static const mode_t S_IRUSR      = mode_t(_S_IREAD);     ///< read by user
static const mode_t S_IWUSR      = mode_t(_S_IWRITE);    ///< write by user
static const mode_t S_IXUSR      = 0x00400000;           ///< does nothing
#   ifndef STRICT_UGO_PERMISSIONS
static const mode_t S_IRGRP      = mode_t(_S_IREAD);     ///< read by *USER*
static const mode_t S_IWGRP      = mode_t(_S_IWRITE);    ///< write by *USER*
static const mode_t S_IXGRP      = 0x00080000;           ///< does nothing
static const mode_t S_IROTH      = mode_t(_S_IREAD);     ///< read by *USER*
static const mode_t S_IWOTH      = mode_t(_S_IWRITE);    ///< write by *USER*
static const mode_t S_IXOTH      = 0x00010000;           ///< does nothing
#   else
static const mode_t S_IRGRP      = 0x00200000;           ///< does nothing
static const mode_t S_IWGRP      = 0x00100000;           ///< does nothing
static const mode_t S_IXGRP      = 0x00080000;           ///< does nothing
static const mode_t S_IROTH      = 0x00040000;           ///< does nothing
static const mode_t S_IWOTH      = 0x00020000;           ///< does nothing
static const mode_t S_IXOTH      = 0x00010000;           ///< does nothing
#   endif
static const mode_t MS_MODE_MASK = 0x0000ffff;           ///< low word
#endif
*/

QSshSocket::QSshSocket(QObject * parent )
    :QThread(parent)
{
    m_host = "";
    m_user = "";
    m_password = "";
    m_port = -1;
    m_loggedIn = false;
    m_session  = NULL;
    m_workingDirectory = ".";

    qRegisterMetaType<QSshSocket::SshError>("QSshSocket::SshError");
#ifdef LIBSSH_STATIC
    ssh_init();
#endif
}

QSshSocket::~QSshSocket()
{
#ifdef LIBSSH_STATIC
    ssh_finalize();
#endif
    m_run = false;
    this->wait();
}

/* A very simple terminal emulator:
   - print data received from the remote computer
   - send keyboard input to the remote computer
*/
int QSshSocket::interactiveShellSession(void)
{
    int rc;
    char buffer[8000];
    int nbytes,totalBytes, nwritten;
    /* Session and terminal initialization skipped */
    ssh_channel channel = ssh_channel_new(m_session);
    if(channel == NULL)
        return SSH_ERROR;
    if((rc = ssh_channel_open_session(channel) )!= SSH_OK){
        ssh_channel_free(channel);
        return rc;
    }
    rc = ssh_channel_request_shell(channel);
    if (rc != SSH_OK){
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return rc;
    }
    QString curCmdStr="login-shell";
    ssh_channel_write(channel,"\n", 1);
    msleep(100);
    while (m_run && m_currentOperation.type == ShellLoop
           && ssh_channel_is_open(channel)
           && !ssh_channel_is_eof(channel)) {
        totalBytes = 0;
        do{
            nbytes = ssh_channel_read_timeout(channel, &buffer[totalBytes], sizeof(buffer) - totalBytes, 0, 200);
            if (nbytes > 0){
                totalBytes += nbytes;
            }
            else if(nbytes == SSH_ERROR){
                ssh_channel_send_eof(channel);
                ssh_channel_close(channel);
                ssh_channel_free(channel);
                return nbytes;
            }
            else if(nbytes == SSH_AGAIN){
                msleep(10);
                qDebug("read again");
            }
        }while(nbytes!=0);
        if(!curCmdStr.isEmpty() || totalBytes > 0 ){
            QString response  = QString::fromUtf8(buffer, totalBytes);
            write(1, buffer, totalBytes);
            memset(buffer, 0, sizeof(buffer));
            emit commandExecuted(curCmdStr, response);
            curCmdStr.clear();
        }
        if ( m_currentOperation.shellCommand.isEmpty() )
        {
            msleep(100);
            continue;
        }
        curCmdStr = m_currentOperation.shellCommand.takeFirst();
        // add exit cmd to shellCmd if want to exit interactive shell
        if(curCmdStr=="exit"){
            break;
        }
        nbytes = curCmdStr.size();
        if (nbytes > 0) {
            nwritten = ssh_channel_write(channel, curCmdStr.toUtf8().data(), nbytes);
            if (nwritten != nbytes) break;
            nwritten = ssh_channel_write(channel,"\n", 1);
        }
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_OK;

}

int QSshSocket::executeOneRemoteCmd(const QString &cmd, QString &response)
{
    int rc;
    ssh_channel channel = ssh_channel_new(m_session);
    if(channel == NULL)
        return SSH_ERROR;
    if((rc = ssh_channel_open_session(channel) )!= SSH_OK){
        ssh_channel_free(channel);
        qDebug(ssh_get_error((m_session)));
        return rc;
    }
    rc = SSH_AGAIN;
    while (rc == SSH_AGAIN)
        rc = ssh_channel_request_exec(channel, cmd.toUtf8().data());

    if (rc != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return rc;
    }

    QByteArray buffer;
    buffer.resize(8000);
    // read in command result
    int totalBytes = 0, newBytes = 0;
    do
    {
        newBytes = ssh_channel_read_timeout(channel, &(buffer.data()[totalBytes]), buffer.size() - totalBytes, 0, 2000);
        if (newBytes > 0)
            totalBytes += newBytes;
    }while (newBytes > 0);

    // close channel
    if(totalBytes==0){
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return SSH_ERROR;
    }
    response = QString(buffer).mid(0,totalBytes);
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_OK;
}

int QSshSocket::executeLogin(void)
{
    int worked;
    if (m_session != NULL) {
        if(ssh_is_connected(m_session))
            ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = NULL;
    }
    if (m_host.isEmpty())
    {
        qDebug("host is empty,set it first");
        return SSH_ERROR;
    }
    m_session = ssh_new();
    if(m_session == NULL){
        qDebug()<<"ssh_new error:"<<tr(ssh_get_error(m_session));
        exit(-1);
    }
    //set logging to verbose so all errors can be debugged if crash happens
    int verbosity = SSH_LOG_PROTOCOL;
    // set the pertinant ssh session options
    ssh_options_set(m_session, SSH_OPTIONS_HOST, m_host.toUtf8().data());
    if(m_user.isEmpty()) m_user = "root";
    ssh_options_set(m_session, SSH_OPTIONS_USER, m_user.toUtf8().data());
//    ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    if(m_timeout != -1)
        ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT, &m_timeout);
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_port);
    // try to connect given host, user, port
    worked = ssh_connect(m_session);
    // if connection is Successful keep track of connection info.
    if (worked != SSH_OK)
    {
        ssh_free(m_session);
        m_session = NULL;
        error(SessionCreationError);
        return worked;
    }

    // check to see if a username and a password have been given
    // check to see if a username and a private key have been given
    if( !m_key.isEmpty() ) {
        ssh_key private_key;
        worked = ssh_pki_import_privkey_base64(m_key.toUtf8().data(), NULL, NULL, NULL, &private_key);
        if(worked == SSH_OK) {
            // try authenticating current user at remote host
            worked = ssh_userauth_publickey(m_session, m_user.toUtf8().data(), private_key);
            // if successful, store user key.//
            if (worked == SSH_AUTH_SUCCESS) {
                m_loggedIn = true;
            }
            else {
                m_key.clear();
            }
        }
        else {
            m_key.clear();
        }
    }
    else if ( !m_password.isEmpty() ) {
        // try authenticating current user at remote host
        worked = ssh_userauth_password(m_session, m_user.toUtf8().data(), m_password.toUtf8().data());
        if (worked == SSH_AUTH_SUCCESS) {
            m_loggedIn = true;
        }
        else if(worked == SSH_AUTH_AGAIN){
            qDebug("login again");
            executeLogin();// login agian
        }
        else {
            qDebug(ssh_get_error(m_session));
            m_password.clear();
        }
    }
    else {
        worked = ssh_userauth_publickey_auto(m_session, NULL, NULL);
        if( worked == SSH_AUTH_SUCCESS ){
            m_loggedIn = true;
        }
        else{
            worked = ssh_userauth_none(m_session, m_user.toUtf8().data());
            if( worked == SSH_AUTH_SUCCESS ){
                m_loggedIn = true;
            }
            else{
                qDebug()<<__func__<< __LINE__;
                qDebug(ssh_get_error(m_session));
            }
        }
    }
    if(!m_loggedIn)
        error(PasswordAuthenticationFailedError);
    else {
        emit loginSuccessful();
    }
    return worked;
}

void QSshSocket::run()
{
    while(m_run)
    {
        {
            // authenticate connection with credentials
            if (!m_loggedIn || m_currentOperation.type == Login){
                executeLogin();
            }
            else if(m_currentOperation.type == ShellLoop){
                interactiveShellSession();
                m_currentOperation.shellCommand.clear();
            }
            // if all ssh setup has been completed, check to see if we have any commands to execute
            else if (m_currentOperation.type == Command || m_currentOperation.type == WorkingDirectoryTest)
            {
                QString response;
//                int rc = executeShellCmd(m_currentOperation.adminCommand, response);
                int rc = executeOneRemoteCmd(m_currentOperation.adminCommand, response);
                if(rc == SSH_OK){
                    if (m_currentOperation.type == WorkingDirectoryTest)
                    {
                        response.replace("\n","");
                        if (response == "exists")
                            m_workingDirectory = m_nextWorkingDir;
                        m_nextWorkingDir = ".";
                        emit workingDirectorySet(m_workingDirectory);
                    }
                    else
                        emit commandExecuted( m_currentOperation.command, response);
                }
            }
            // if all ssh setup has been completed, check to see if we have any file transfers to execute
            else if (m_currentOperation.type == Pull)
            {
                ssh_scp scpSession = ssh_scp_new(m_session,SSH_SCP_READ, m_currentOperation.remotePath.toUtf8().data());
                if (scpSession == NULL)
                    error(ScpChannelCreationError);

                // attempt to initialize new scp session.
                int scpInitialized = ssh_scp_init(scpSession);
                if(scpInitialized != SSH_OK)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpChannelCreationError);
                }


                // attempt to authorize new scp pull
                if (ssh_scp_pull_request(scpSession) != SSH_SCP_REQUEST_NEWFILE)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpPullRequestError);
                }

                // accept authorization
                ssh_scp_accept_request(scpSession);


                // get remote file size
                int size = ssh_scp_request_get_size(scpSession);

                // resize buffer, read remote file into buffer
                QByteArray buffer;
                buffer.resize(size);

                // if an error happens while reading, close the scp session and return
                if (ssh_scp_read(scpSession, buffer.data() , size) == SSH_ERROR)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpReadError);
                }

                // loop until eof flag
                if  (ssh_scp_pull_request(scpSession)  != SSH_SCP_REQUEST_EOF)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpReadError);
                }

                //close scp session
                ssh_scp_close(scpSession);
                ssh_scp_free(scpSession);

                // open up local file and write contents of buffer to it.
                QFile file(m_currentOperation.localPath);
                file.open(QIODevice::WriteOnly);
                file.write(buffer);
                file.close();

                emit pullSuccessful(m_currentOperation.localPath,m_currentOperation.remotePath);

            }
            else if (m_currentOperation.type == Push)
            {
                // attempt to create new scp from ssh session.
                ssh_scp scpSession = ssh_scp_new(m_session,SSH_SCP_WRITE, m_currentOperation.remotePath.toUtf8().data());

                // if creation failed, return
                if (scpSession == NULL)
                    error(SocketError);


                // attempt to initialize new scp session.
                int scpInitialized = ssh_scp_init(scpSession);


                // if failed, close scp session and return.
                if(scpInitialized != SSH_OK)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpChannelCreationError);
                }


                // open the local file and check to make sure it exists
                // if not, close scp session and return.
                QFile file(m_currentOperation.localPath);
                if (!file.exists())
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpFileNotCreatedError);
                }

                // if the file does exist, read all contents as bytes
                file.open(QIODevice::ReadOnly);
                QByteArray buffer =file.readAll();
                file.close();

                // attempt to authorize pushing bytes over scp socket
                // if this fails, close scp session and return.
                if (ssh_scp_push_file(scpSession, m_currentOperation.remotePath.toUtf8().data(), buffer.size(), S_IRUSR | S_IWUSR) != SSH_OK)
                {
                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpPushRequestError);
                }


                // once authorized to push bytes over scp socket, start writing
                // if an error is returned,  close scp session and return.
                if ( ssh_scp_write(scpSession,buffer.data(), buffer.size()) != SSH_OK)
                {

                    ssh_scp_close(scpSession);
                    ssh_scp_free(scpSession);
                    error(ScpWriteError);
                }


                // close scp session and return.
                ssh_scp_close(scpSession);
                ssh_scp_free(scpSession);

                emit pushSuccessful(m_currentOperation.localPath,m_currentOperation.remotePath);
            }
            else {
                msleep(10);
            }
        }
    }
    if (m_session != NULL)
    {
        if(ssh_is_connected(m_session))
            ssh_disconnect(m_session);
        ssh_free(m_session);
    }
    m_session = NULL;
    emit disconnected();

}
void QSshSocket::disconnectFromHost()
{
    m_run = false;
    this->wait();
    m_host = "";
    m_user = "";
    m_password = "";
    m_key = "";
    m_port = -1;
    m_loggedIn = false;
}

void QSshSocket::setKey(QString key)
{
    m_key = key;
}

void QSshSocket::setConnectHost(QString host, int port)
{
    m_host = host;
    m_port = port;
}
void QSshSocket::login(QString user, QString password)
{
    m_run = true;
    start();
    m_user = user;
    m_password = password;
    m_currentOperation.type = Login;
    msleep(100);
    m_currentOperation.type = Unkonw;
}
void QSshSocket::setTimeout(long timeout)
{
    m_timeout = timeout;
}
void QSshSocket::executeCommand(QString command)
{
    if (m_workingDirectory != ".")
        m_currentOperation.adminCommand = "cd " + m_workingDirectory + "; "  + command;
    else
        m_currentOperation.adminCommand = command ;

    m_currentOperation.command = command;
    m_currentOperation.type = Command;
    msleep(100);
    m_currentOperation.type = Unkonw;
}
void QSshSocket::add2ShellCommand(QString command)
{
    if(m_currentOperation.type != ShellLoop)
        return;
    m_currentOperation.shellCommand << command.split(";", QString::SkipEmptyParts);
}

void QSshSocket::pullFile(QString localPath, QString remotePath)
{
    m_currentOperation.localPath = localPath;
    if (QFileInfo(remotePath).isAbsolute())
        m_currentOperation.remotePath = remotePath;
    else
        m_currentOperation.remotePath = m_workingDirectory + "/" + remotePath;
    m_currentOperation.type = Pull;
    msleep(100);
    m_currentOperation.type = Unkonw;
}

void QSshSocket::pushFile(QString localPath, QString remotePath)
{
    m_currentOperation.localPath = localPath;
    if (QFileInfo(remotePath).isAbsolute())
        m_currentOperation.remotePath = remotePath;
    else
        m_currentOperation.remotePath = m_workingDirectory + "/" + remotePath;
    m_currentOperation.type = Push;
    msleep(100);
    m_currentOperation.type = Unkonw;
}

void QSshSocket::setWorkingDirectory(QString path)
{
    m_nextWorkingDir = path;
    m_currentOperation.adminCommand = "[ -d " + m_nextWorkingDir +" ] && echo 'exists'";
    m_currentOperation.type = WorkingDirectoryTest;
    msleep(100);
    m_currentOperation.type = Unkonw;
}

bool QSshSocket::isLoggedIn()
{
    return m_loggedIn;
}

QString QSshSocket::user(){return m_user;}
QString QSshSocket::host(){return m_host;}
int QSshSocket::port(){return m_port;}