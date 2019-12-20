#ifndef QSSHSOCKET_H
#define QSSHSOCKET_H
#include "libssh/libssh.h"
#include <QByteArray>
#include <QThread>
#include <QVector>
#include <QFile>
#include <sys/stat.h>
#include <sys/types.h>
#include <QDebug>
#include <QKeyEvent>

class QSshSocket: public QThread
{
    Q_OBJECT
public:

    enum SshError
    {
        /*! \brief There was trouble creating a socket. This was most likely due to the lack of an internet connection.*/
        SocketError,
        /*! \brief The ssh session could not be created due to inability to find the remote host.*/
        SessionCreationError,
        /*! \brief An ssh channel could not be created for the previous operation.*/
        ChannelCreationError,
        /*! \brief An scp channel could not be created for the previous file transfer operation.*/
        ScpChannelCreationError,
        /*! \brief There was an error requesting a pull file transfer.*/
        ScpPullRequestError,
        /*! \brief There was an error requesting a push file transfer.*/
        ScpPushRequestError,
        /*! \brief The destination file for the previous transfer does not exist.*/
        ScpFileNotCreatedError,
        /*! \brief There was an error reading a remote file. This could possibly be due to user permissions.*/
        ScpReadError,
        /*! \brief There was an error writing to a remote file. This could possibly be due to user permissions.*/
        ScpWriteError,
        /*! \brief The credentials of a user on the remote host could not be authenticated.*/
        PasswordAuthenticationFailedError
    };

    /*!
        \brief The constructor.
    */
    explicit QSshSocket(QObject * parent = 0);

    /*!
        \brief The deconstructor.
    */
    ~QSshSocket();

    /*!
        \param host The hostname to establish an ssh connection with.
        \param port The port to establish an ssh connection over.
        \brief This function connects this socket to the specified host over the specified port. On success, the signal connected is emitted while error is emmited on failure.
    */
    void setConnectHost(QString host, int port =22);

    /*!
        \brief This function disconnects the socket from the current host (if there is one. On success, the signal disconnected is emitted while error is emmited on failure.
    */
    void disconnectFromHost();

    /*!
        \param command The command to be executed.
        \brief This function executes a remote command on the connected host. If not connected to a remote host, the command is not executed.
        On success, the signal commandExecuted is emitted while error is emmited on failure.
    */
    void executeCommand(QString command);
    inline void clearShellCmd(void){ m_currentOperation.shellCommand.clear(); }
    void add2ShellCommand(QString command);
    void enter2shell(void){  m_currentOperation.type = ShellLoop; }
    /*!
        \brief Returns the hostname of the remote machine this socket is connected to. If not connected to a remote host, this returns "".
    */
    QString host();

    /*!
        \brief Returns whether or not a user has been logged in at remote host.
    */
    bool isLoggedIn();

    /*!
        \param user The username to login with.
        \param password The password of the account for the specified username.
        \brief This function to login to the currently connected host given credentials.
        On success, the signal authenticated is emitted while error is emmited on failure.
    */
    void login(QString user, QString password="");

    /*!
        \param key The private key to login with.
        \brief This function to login to the currently connected host given private key.
        On success, the signal authenticated is emitted while error is emmited on failure.
    */
    void setKey(QString key);

    /*!
        \param timeout Connection timeout in sec.
        \brief Set timeout for ssh connection in seconds.
    */
    void setTimeout(long timeout);

    /*!
        \brief Returns the port of the current connection. If not connected to a remote host, this returns -1.
    */
    int port();

    /*!
        \param localPath A path to a file stored on the local machine.
        \param remotePath A path to a file stored on the remote machine.
        \brief This function attempts to pull a remote file from the connected host to a local file. The local file does not need to be created beforehand.
        On success, the signal pullSuccessful is emitted while error is emmited on failure.
        If not connected to a remote host, or if the transfer was unsuccessful, the signal error is emitted.
    */
    void pullFile(QString localPath, QString remotePath);

    /*!
        \param localPath A path to a file stored on the local machine.
        \param remotePath A path to a file stored on the remote machine.
        \brief This function attempts to pull a remote file from the connected host to a local file. The local file does not need to be created beforehand.
        On success, the signal pushSuccessful is emitted while error is emmited on failure.
        If not connected to a remote host, or if the transfer was unsuccessful, the signal error is emitted.
    */
    void pushFile(QString localPath, QString remotePath);

    /*!
        \param path A relative or absolute path to a directory on the remote host.
        \brief This function attempts to set the working directory of the connection to path and emits workingDirectorySet upon completion.
        If workingDirectorySet indicates no change in the working directory, the path could not be found.
        If not connected to a remote host the signal error will be emitted.
    */
    void setWorkingDirectory(QString path);

    /*!
        \brief Returns the username of the current authenticated user on the remote host. If not connected to a remote host, or if a user has not been authenticated this returns "".
    */
    QString user();
protected:
    bool eventFilter(QObject *obj, QEvent *e);

signals:

    /*!
        \brief This signal is emitted when this class has been properly disconnected from a remote host.
    */
    void disconnected();

    /*!
        \param error The type of error that occured.
        \brief This signal is emitted when an error occurs.
    */
    void error(QSshSocket::SshError error);

    /*!
        \param command The command that was executed on the remote host.
        \param response The response to the command that was executed.
        \brief This signal is emitted when a response from the remote host is received regarding a command.
    */
    void commandExecuted(QString command,QString response);

    /*!
        \brief This signal is emitted when a user has been loggen in to the remote host."
    */
    void loginSuccessful();

    /*!
        \param localFile The path to a local file that the remote file was pulled to.
        \param remoteFile The path to a file pulled from the remote host.
        \brief This signal is emitted when a remote file is successfully transfered to a local file.
    */
    void pullSuccessful(QString localFile, QString remoteFile);

    /*!
        \param localFile The path to a local file pushed to the remote host.
        \param remoteFile The path to a remote file that the local file was pushed to.
        \brief This signal is emitted when a local file is successfully transfered to a remote file.
    */
    void pushSuccessful(QString localFile, QString remoteFile);

    /*!
        \param cwd The current working directory of the session on the remote host.
        \brief This signal is emitted when a current working directory is set.
    */
    void workingDirectorySet(QString cwd);
private slots:
    void run();

private:

    enum SSHOperationType
    {
        Login,
        ShellLoop,
        Command,
        WorkingDirectoryTest,
        Pull,
        Push,
        Unkonw,
    };

    struct SSHOperation
    {
        SSHOperationType type;
        QString adminCommand,command, localPath, remotePath;
        QStringList shellCommand;
    };
    int executeLogin(void);
    int executeOneRemoteCmd(const QString &cmd, QString &response);
    int interactiveShellSession();
    int executeScpPush();
    int executeScpPull();

    int m_port;
    long m_timeout = -1;
    bool m_loggedIn ;
    QString m_workingDirectory,m_nextWorkingDir,m_user, m_host,m_password,m_key;
    SSHOperation m_currentOperation;
    ssh_session m_session;
    bool m_connected,m_run;
    ssh_channel m_channel;
};


#endif // QSSHSOCKET_H
