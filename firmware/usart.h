unsigned int usart_write(const char* c, unsigned int length);
unsigned int usart_print(const char* c);
unsigned int usart_printf(const char* fmt, ...);
void usart_init(void);

typedef void (*on_line_recv_cb)(const char* c, unsigned int length);
extern on_line_recv_cb usart_on_line_recv;
