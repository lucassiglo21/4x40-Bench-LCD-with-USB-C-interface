//defines and prototypes

char input_buffer_pointer;
char input_buffer_size;
char input_buffer_char;
char input_buffer[64];

//extern
char cdc_rx_len;

//prototypes
char getnewbuffer(void);
char getnextbyte(void);
char getsUSBUSART(char *buffer, unsigned char len);

//main
char getnewbuffer(void)
{
if(getsUSBUSART(input_buffer,64))
	{
	input_buffer_size=cdc_rx_len;
	input_buffer_pointer=0;
	return 1;
	}
return 0;
}

char getnextbyte(void)
{
//if it's at the end of the buffer it needs to get a new one
if(input_buffer_size<=input_buffer_pointer)
	{
	if(!getnewbuffer()){return 0;};	//couldn't get a new char
	input_buffer_pointer=0;			//could, so reset pointer
	}
input_buffer_char=input_buffer[input_buffer_pointer];
input_buffer_pointer++;
return 1;
}
