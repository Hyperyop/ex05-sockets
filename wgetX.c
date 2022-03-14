/**
 *  Jiazi Yi
 *
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 *
 * Cisco Systems
 * ppfister@cisco.com
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "url.h"
#include "wgetX.h"
#define MAXRCVLEN 2048
int main(int argc, char* argv[]) {
    url_info info;
    const char * file_name = "received_page";
    if (argc < 2) {
	fprintf(stderr, "Missing argument. Please enter URL.\n");
	return 1;
    }

    char *url = argv[1];

    // Get optional file name
    if (argc > 2) {
	file_name = argv[2];
    }

    // First parse the URL
    int ret = parse_url(url, &info);
    if (ret) {
	fprintf(stderr, "Could not parse URL '%s': %s\n", url, parse_url_errstr[ret]);
	return 2;
    }

    //If needed for debug
    //print_url_info(&info);

    // Download the page
    struct http_reply reply;

    ret = download_page(&info, &reply);
    if (ret) {
	return 3;
    }
    char *response;
    // Now parse the responses
    int tries=0;
    while(test_for_redirect(&reply)){
        tries++;
        if(tries>=10){
            fprintf(stderr, "Redirection depth exceeded (10) \n");
	        return 2;
        }
        response = read_http_reply(&reply);
        char *tmp;
        tmp = next_line(response,strlen(response));
        *tmp = '\0';
        if (response == NULL) {
	    fprintf(stderr, "Could not parse http reply\n");
	    return 4;
    }
        response = response +10;
        while(isspace(*response)) response++;
        printf("the url is %s \n",response);
        if (*response=='/'){
            response++;
            info.path=response;
            ret = download_page(&info, &reply);
            if (ret) {
	        return 3;
            }
        }
        else{
            ret = parse_url(response, &info);
            if (ret) {
	        fprintf(stderr, "Could not parse URL '%s': %s\n", response, parse_url_errstr[ret]);
	        return 2;
            }
            ret = download_page(&info, &reply);
            if (ret) {
	        return 3;
            }
        }
    }
    
    response = read_http_reply(&reply);
    if (response == NULL) {
	fprintf(stderr, "Could not parse http reply\n");
	return 4;
    }

    // Write response to a file
    write_data(file_name, response, reply.reply_buffer + reply.reply_buffer_length - response);

    // Free allocated memory
    free(reply.reply_buffer);

    // Just tell the user where is the file
    fprintf(stderr, "the file is saved in %s.", file_name);
    return 0;
}

int download_page(url_info *info, http_reply *reply) {


    /*
     * To be completed:
     *   You will first need to resolve the hostname into an IP address.
     *
     *   Option 1: Simplistic
     *     Use gethostbyname function.
     *
     *   Option 2: Challenge
     *     Use getaddrinfo and implement a function that works for both IPv4 and IPv6.
     *
     */
    // struct hostent *hostinfo;
    // hostinfo = gethostbyname(info->host);
    // char *test = inet_ntoa(*( struct in_addr*)( hostinfo->h_addr_list[0]));
    // printf("%s\n", test);
    // int len, mysocket;
    // struct sockaddr_in dest;
    // mysocket = socket(AF_INET, SOCK_STREAM, 0);
    // memset(&dest, 0, sizeof(dest));
    // dest.sin_family = AF_INET;
    // dest.sin_addr = *( struct in_addr*)( hostinfo->h_addr_list[0]);
    // dest.sin_port = htons(info->port);
    // if (connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr))) {
	// fprintf(stderr, "Could not connect: %s\n", strerror(errno));
	// return -1;
    // }
    int mysocket,len,s;
    char port[6];
    snprintf(port,6,"%d",info->port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM ; /* stream socket */ 
    struct addrinfo *result,*rp;
    s = getaddrinfo(info->host, port,&hints, &result); //this block was inspired by the man example entry
    if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        mysocket = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
        if (mysocket == -1)
            continue;
        
        if (connect(mysocket,rp->ai_addr,rp->ai_addrlen)!=-1)
            break;
        fprintf(stderr, "Couldn't connect to IP adress\n");     
        close(mysocket);            /* Success */
    }
    if (rp==NULL){ 
        fprintf(stderr, "Couldn't connect to IP adress\n");
        exit(EXIT_FAILURE);
    }
    free(result);

    
    
    /*
     * To be completed:
     *   Next, you will need to send the HTTP request.
     *   Use the http_get_request function given to you below.
     *   It uses malloc to allocate memory, and snprintf to format the request as a string.
     *
     *   Use 'write' function to send the request into the socket.
     *
     *   Note: You do not need to send the end-of-string \0 character.
     *   Note2: It is good practice to test if the function returned an error or not.
     *   Note3: Call the shutdown function with SHUT_WR flag after sending the request
     *          to inform the server you have nothing left to send.
     *   Note4: Free the request buffer returned by http_get_request by calling the 'free' function.
     *
     */
    char *bufferMessage = http_get_request(info);
    if(write(mysocket, bufferMessage, strlen(bufferMessage))<-1){
       fprintf(stderr, "Could not write");
	    return -1;
    }
    shutdown(mysocket ,SHUT_WR);
    free(bufferMessage);
    
    /*
     * To be completed:
     *   Now you will need to read the response from the server.
     *   The response must be stored in a buffer allocated with malloc, and its address must be save in reply->reply_buffer.
     *   The length of the reply (not the length of the buffer), must be saved in reply->reply_buffer_length.
     *
     *   Important: calling recv only once might only give you a fragment of the response.
     *              in order to support large file transfers, you have to keep calling 'recv' until it returns 0.
     *
     *   Option 1: Simplistic
     *     Only call recv once and give up on receiving large files.
     *     BUT: Your program must still be able to store the beginning of the file and
     *          display an error message stating the response was truncated, if it was.
     *
     *   Option 2: Challenge
     *     Do it the proper way by calling recv multiple times.
     *     Whenever the allocated reply->reply_buffer is not large enough, use realloc to increase its size:
     *        reply->reply_buffer = realloc(reply->reply_buffer, new_size);
     *
     *
     */
    int buffer_length=0;
    reply->reply_buffer = (char *) malloc(MAXRCVLEN);
    len = recv(mysocket, reply->reply_buffer, MAXRCVLEN , 0);
    buffer_length+=len;
    if (len < 0) {
        fprintf(stderr, "recv returned error: %s\n", strerror(errno));
        return -1;
    }
    while(len>0){
    reply->reply_buffer =(char *) realloc(reply->reply_buffer,buffer_length+MAXRCVLEN);
    len = recv(mysocket, reply->reply_buffer+buffer_length,MAXRCVLEN , 0);
    buffer_length+=len;
    if (len < 0) {
        fprintf(stderr, "recv returned error: %s\n", strerror(errno));
        return -1;}
    // }else{
    //     printf("cool boi %s\n",reply->reply_buffer);
    // }
    }
    reply->reply_buffer_length=buffer_length;
    close(mysocket);

    return 0;
}

void write_data(const char *path, const char * data, int len) {
    FILE *ptr;
    ptr=fopen(path,"w");
    if(ptr==NULL){
        fprintf(stderr, "Could not open file at %s\n",path);
        return;
    }
    fprintf(ptr,"%s",data);
    fclose(ptr);
}

char* http_get_request(url_info *info) {
    char * request_buffer = (char *) malloc(100 + strlen(info->path) + strlen(info->host));
    snprintf(request_buffer, 1024, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    info->path, info->host);
    return request_buffer;
}

char *next_line(char *buff, int len) {
    if (len == 0) {
	return NULL;
    }

    char *last = buff + len - 1;
    while (buff != last) {
	if (*buff == '\r' && *(buff+1) == '\n') {
	    return buff;
	}
	buff++;
    }
    return NULL;
}
int test_for_redirect(struct http_reply *reply){
    // Let's first isolate the first line of the reply
    char *status_line = next_line(reply->reply_buffer, reply->reply_buffer_length);
    if (status_line == NULL) {
	fprintf(stderr, "Could not find status\n");
	return 0;
    }
    *status_line = '\0'; // Make the first line is a null-terminated string

    // Now let's read the status (parsing the first line)
    int status;
    double http_version;
    int rv = sscanf(reply->reply_buffer, "HTTP/%lf %d", &http_version, &status);
    if (rv != 2) {
	fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
	return 0;
    }
    char *buf = status_line + 2;
    char *tmp;
    char *location;
    int len = reply->reply_buffer_length+(reply->reply_buffer-buf);
    if (status>=300 && status < 400){
        printf("Got a redirect request \n attempting to follow it\n");
        while(buf!=NULL){
        location = strstr(buf,"Location:");
        if(location!=NULL){
            return 1;
        }
        tmp=next_line(buf,len);
        if(buf==tmp) return 0;
        tmp+=2;
        len-=tmp-buf;
        buf=tmp;
        }
        
    }return 0;
}
char *read_http_reply(struct http_reply *reply) {

    // Let's first isolate the first line of the reply
    char *status_line = next_line(reply->reply_buffer, reply->reply_buffer_length);
    if (status_line == NULL) {
	fprintf(stderr, "Could not find status\n");
	return NULL;
    }
    *status_line = '\0'; // Make the first line is a null-terminated string

    // Now let's read the status (parsing the first line)
    int status;
    double http_version;
    int rv = sscanf(reply->reply_buffer, "HTTP/%lf %d", &http_version, &status);
    if (rv != 2) {
	fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
	return NULL;
    }
    char *buf = status_line + 2;
    char *tmp;
    char *location;
    int len = reply->reply_buffer_length+(reply->reply_buffer-buf);
    if (status>=300 && status < 400){
        while(buf!=NULL){
        location = strstr(buf,"Location:");
        if(location!=NULL){
            return location;
        }
        tmp=next_line(buf,len);
        if(buf==tmp) return buf+2;
        tmp+=2;
        len-=tmp-buf;
        buf=tmp;
        }

    }else
    if (status != 200) {
	fprintf(stderr, "Server returned status %d (should be 200)\n", status);
	return NULL;
    }
    while(buf!=NULL){
        tmp=next_line(buf,len);
        if(buf==tmp) return buf+2;
        tmp+=2;
        len-=tmp-buf;
        buf=tmp;
        }
    
    

    /*
     * To be completed:
     *   The previous code only detects and parses the first line of the reply.
     *   But servers typically send additional header lines:
     *     Date: Mon, 05 Aug 2019 12:54:36 GMT<CR><LF>
     *     Content-type: text/css<CR><LF>
     *     Content-Length: 684<CR><LF>
     *     Last-Modified: Mon, 03 Jun 2019 22:46:31 GMT<CR><LF>
     *     <CR><LF>
     *
     *   Keep calling next_line until you read an empty line, and return only what remains (without the empty line).
     *
     *   Difficul challenge:
     *     If you feel like having a real challenge, go on and implement HTTP redirect support for your client.
     *
     */




    return buf;
}
