void usart_write(const char* c, unsigned int length);
unsigned int usart_readline(char* buffer, unsigned int length);
void usart_print(const char* c);
void configure_usart(void);

typedef void (*on_line_recv_cb)(const char* c, unsigned int length);
extern on_line_recv_cb on_line_recv;
