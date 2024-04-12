#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD //HTTP请求方法 
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE //主状态机的状态
    {
        CHECK_STATE_REQUESTLINE = 0,  //正在分析请求行
        CHECK_STATE_HEADER,  //正在分析请求头部字段
        CHECK_STATE_CONTENT  //正在分析请求体
    };
    enum HTTP_CODE    //HTTP请求的处理结果
    {
        NO_REQUEST,  //请求不完整，需要继续读取数据
        GET_REQUEST, //获得完整的客户请求
        BAD_REQUEST, //客户请求语法出错
        NO_RESOURCE, //没有资源
        FORBIDDEN_REQUEST, //客户对资源没有足够的访问权限
        FILE_REQUEST,  //文件请求，获取文件请求成功
        INTERNAL_ERROR,  //表示服务器内部错误
        CLOSED_CONNECTION   //表示客户端已经关闭了连接
    };
    enum LINE_STATUS   //从状态机状态，某一行的状态
    {
        LINE_OK = 0,  //读取到完整的行
        LINE_BAD,     //行出错，语法错误等
        LINE_OPEN     //行数据尚不完整
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();  //初始化状态机相关内容
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;  //epoll事件描述符，所有的usr共享，因此是静态的
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;  //http连接的socket
    sockaddr_in m_address;   //socket的地址

    char m_read_buf[READ_BUFFER_SIZE]; //读缓冲区
    int m_read_idx; //已读到缓冲区的数据尾部
    int m_checked_idx; //当前正在分析的字符在读缓冲区的位置
    int m_start_line;  //当前正在解析行的起始位置

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;

    METHOD m_method;   //请求方法
    char m_real_file[FILENAME_LEN];
    char *m_url;  //请求的url
    char *m_version;  //http版本
    char *m_host;    //客户主机
    int m_content_length;  //请求体长度
    bool m_linger;   //是否持续TCP连接

    char *m_file_address;  //文件映射后在内存的地址
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;  //资源的根目录

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
