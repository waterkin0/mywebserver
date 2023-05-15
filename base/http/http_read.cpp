#include "http.h"

char* http::get_line(int n) {
    return readbuf + n;
}

HTTP_CODE http::process_read() {
    LINE_STATE line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;

    //主从都可以正常处理，说明当前http报文还没处理完，持续循环运行
    int line_start = 0; //每一行开始的地方
    while((mainstate == CHECK_STATE_CONTENT && line_state == LINE_OK)||((line_state = parse_line()) == LINE_OK)) {
        text = get_line(line_start);//读取一行
        // cout << text << endl;
        line_start = checksize_now;
        switch (mainstate)
        {
            case CHECK_STATE_REQUESTLINE:
                ret = parse_request_line(text);//解析请求行
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            case CHECK_STATE_HEADER:
                ret = parse_headers(text);//解析请求头，get就在这里结束了
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST)
                    return do_request();
                break;
            case CHECK_STATE_CONTENT:
                ret = parse_content(text);//解析消息体，post在这结束
                if (ret == GET_REQUEST)
                    return do_request();
                line_state = LINE_OPEN;
                break;
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HTTP_CODE http::do_request() {
    strcpy(httproot_now, html_root);
    int len=strlen(html_root);
    const char *t = strrchr(myhttp.url, '/'); 

    if(*(t + 1) == '0') {
        char tmp[] = "/register.html";
        strcpy(httproot_now + len, tmp);//注意这里这个httproot_now别太长了，httproot_now和tmp长度大于FILENAME_SIZE就要增大FILENAME_SIZE或者改为strncpy了
    }
    else if(*(t + 1) == '1') {
        char tmp[] = "/login.html";
        strcpy(httproot_now + len, tmp);
    }
    else{
        strncpy(httproot_now + len, myhttp.url, FILENAME_SIZE - len - 1);
    }
    if(stat(httproot_now, &myhttp.file_stat) < 0) //文件不存在
        return NO_RESOURCE;
    if(!(myhttp.file_stat.st_mode & S_IROTH)) //不是可读文件
        return FORBIDDEN_REQUEST;
    if(S_ISDIR(myhttp.file_stat.st_mode)) //请求的文件是目录，出错
        return BAD_REQUEST;
    //进行mmap映射
    int fd=open(httproot_now, O_RDONLY);//只读方式打开
    myhttp.file_address = (char*)mmap(NULL, myhttp.file_stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);

    return FILE_REQUEST;
}

LINE_STATE http::parse_line() { //从状态机，读取一行并处理，判断请求是否正确
    char tmp;
    for(; checksize_now < readsize_now; checksize_now++) {
        tmp = readbuf[checksize_now]; //http每一行以\r\n结束,如果确定是这个，将其改为\0，主状态机以字符串读取
        if(tmp == '\r') {
            if(checksize_now + 1 == readsize_now) { //下一个就是当前读取的地方了，继续接收
                return LINE_OPEN;
            }
            else if(readbuf[checksize_now + 1] == '\n') {
                readbuf[checksize_now++] = '\0';
                readbuf[checksize_now++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;//语法错误
        }
        else if(tmp == '\n') {//上一个末尾是\r，这次开头是\n
            if(checksize_now > 1 && readbuf[checksize_now - 1] == '\r') {
                readbuf[checksize_now - 1] = '\0';
                readbuf[checksize_now++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HTTP_CODE http::parse_request_line(char *text) {
    char *httpversion = 0;
    myhttp.url = strpbrk(text," \t");//查找空格或者\t出现的第一个位置
    if(!myhttp.url) return BAD_REQUEST;
    //将该位置改为\0，用于将前面数据取出
    *myhttp.url = '\0';
    myhttp.url ++;

    //判断是GET还是POST
    char *method=text;
    if(strcasecmp(method,"GET")==0)
        myhttp.method=GET;
    else if(strcasecmp(method,"POST")==0)
        myhttp.method=POST;
    else
        return BAD_REQUEST;
    myhttp.url += strspn(myhttp.url," \t");//上面只跳过了一个空格或者\t，现在全部跳过

    //判断HTTP版本,此处将中间的内容跳过了,原因是因为中间内容会进行更改,留到最后处理,本项目只支持1.1
    httpversion = strpbrk(myhttp.url," \t");
    if(!httpversion) return BAD_REQUEST;
    *httpversion = '\0';
    httpversion ++;
    httpversion += strspn(httpversion," \t");
    if(strcasecmp(httpversion, "HTTP/1.1")!=0)
        return BAD_REQUEST;

    //对请求资源前7个字符进行判断,确定是不是http://
    if(strncasecmp(myhttp.url,"http://",7)==0){
        myhttp.url += 7;
        myhttp.url = strchr(myhttp.url,'/');
    }
    //同样增加https情况
    if(strncasecmp(myhttp.url,"https://",8)==0){
        myhttp.url += 8;
        myhttp.url = strchr(myhttp.url,'/');
    }

    //一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
    if(!myhttp.url || myhttp.url[0] != '/')
        return BAD_REQUEST;

    //当url为/时，显示欢迎界面
    if(strlen(myhttp.url) == 1)
        strcat(myhttp.url, "main.html");    
    
    //请求行处理完毕，将主状态机转移处理请求头
    mainstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE http::parse_headers(char *text) {
    //判断是空行还是请求头
    if(text[0] == '\0') {
        if(myhttp.method == POST) {//POST需要跳转到消息体处理状态
            mainstate = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;//GET就处理完毕了
    }
    //解析请求头部连接字段
    else if(strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        //跳过空格和\t字符
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0) {
            myhttp.alive = true;
        }
    }
    //解析请求头部内容长度字段，只有post有
    else if(strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        myhttp.contentsize = atol(text);
    }
    //解析请求头部HOST字段
    else if(strncasecmp(text,"Host:",5)==0) {
        text += 5;
        text += strspn(text," \t");
        myhttp.host = text;
    }
    else{
        LOG_WRITE("unknown header:" + string(text));
        //cout << "unknow header: " << text << endl;
    }
    return NO_REQUEST;
}

HTTP_CODE http::parse_content(char *text) { //仅post会使用
    //判断buffer中是否读取了消息体
    if(readsize_now >= (myhttp.contentsize + checksize_now)) {
        text[myhttp.contentsize]='\0';
        myhttp.postdata = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}