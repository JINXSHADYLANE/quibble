#include "http.h"

#import "asi-http-request/ASIHTTPRequest.h"

void http_get(const char* addr, bool get_header, HttpCallback cb) {
    // We don't support headers for now
    assert(!get_header);
    
    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String: addr]];
    __block ASIHTTPRequest* request = [ASIHTTPRequest requestWithURL:url];
    
    [request setCompletionBlock:^{
        NSData* data = [request responseData];
        (*cb)([request responseStatusCode], [data bytes], [data length], NULL, 0);
    }];
    
    [request setFailedBlock:^{
        (*cb)(-1, NULL, 0, NULL, 0);
    }];
    
    [request startAsynchronous];
}

void http_post(const char* addr, bool get_header,
               const char* data, const char* content_type, HttpCallback cb) {
    
    assert(!get_header);
    
    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String: addr]];
    __block ASIHTTPRequest* request = [ASIHTTPRequest requestWithURL:url];
    
    NSData* post_data = [NSData dataWithBytes:data length:strlen(data)+1];
    [request appendPostData:post_data];
    
    [request setRequestMethod:@"POST"];
    if(content_type)
        [request addRequestHeader:@"Content-Type" value:[NSString stringWithUTF8String:content_type]];
    
    [request setCompletionBlock:^{
        NSData* data = [request responseData];
        (*cb)([request responseStatusCode], [data bytes], [data length], NULL, 0);
    }];
    
    [request setFailedBlock:^{
        (*cb)(-1, NULL, 0, NULL, 0);
    }];
    
    [request startAsynchronous];
}

void http_put(const char* addr, void* data, size_t size, 
              bool get_header, HttpCallback cb) {
    
    assert(!get_header);
    
    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String: addr]];
    __block ASIHTTPRequest* request = [ASIHTTPRequest requestWithURL:url];
    
    NSData* post_data = [NSData dataWithBytes:data length:size];
    [request appendPostData:post_data];
    
    [request setRequestMethod:@"PUT"];
    
    [request setCompletionBlock:^{
        NSData* data = [request responseData];
        (*cb)([request responseStatusCode], [data bytes], [data length], NULL, 0);
    }];
    
    [request setFailedBlock:^{
        (*cb)(-1, NULL, 0, NULL, 0);
    }];
    
    [request startAsynchronous];
}

void http_delete(const char* addr, bool get_header, HttpCallback cb) {
    // We don't support headers for now
    assert(!get_header);
    
    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String: addr]];
    __block ASIHTTPRequest* request = [ASIHTTPRequest requestWithURL:url];
    
    [request setRequestMethod:@"DELETE"];
    
    [request setCompletionBlock:^{
        NSData* data = [request responseData];
        (*cb)([request responseStatusCode], [data bytes], [data length], NULL, 0);
    }];
    
    [request setFailedBlock:^{
        (*cb)(-1, NULL, 0, NULL, 0);
    }];
    
    [request startAsynchronous];

}
