#include "SerialCom.h"

SerialCom::SerialCom()
{
	// Open File Descriptor 
	const char device[] = "/dev/ttyUSB0";

	m_dev = open(device, O_RDWR| O_NONBLOCK);

	if (m_dev < 0)
	{
		std::cout << "Error "
			 << errno << " opening " 
			 << device << ": " 
			 << strerror (errno) << std::endl;
	}

	// Configure port
	struct termios tty;
	memset (&tty, 0, sizeof(tty));

	if (tcgetattr (m_dev, &tty) != 0 )
	{
		std::cout << "Error " << errno 
		     	  << " from tcgetattr: " 
		     	  << strerror(errno) << std::endl;
	}

	// Set Baud Rate 
	cfsetospeed (&tty, B115200);
	cfsetispeed (&tty, B115200);

	// Set port settings
	tty.c_cflag     &=  ~CSTOPB;	//
	tty.c_cflag     &=  ~CRTSCTS;   // no flow control
	tty.c_lflag     =   0;          // no signaling chars, no echo, no canonical processing
	tty.c_oflag     =   0;      	// no remapping, no delays
	tty.c_cc[VMIN]  =   0;      	// read doesn't block
	tty.c_cc[VTIME] =   5;      	// 0.5 seconds read timeout

	tty.c_cflag     |=  CREAD | CLOCAL; // turn on READ & ignore ctrl lines
	tty.c_iflag     &=  ~IXOFF;			// turn off s/w flow ctrl

	// set terminal to raw mod
	cfmakeraw(&tty);
	// Flush Port, then applies attributes 
	tcflush(m_dev, TCIFLUSH );

	if (tcsetattr (m_dev, TCSANOW, &tty) != 0)
	{
		std::cout << "Error " 
				  << errno 
				  << " from tcsetattr" 
				  << std::endl;
	}

	// Allocate memory for read and right buffer
	memset(&m_read_buffer, '\0', sizeof(m_read_buffer));

	m_cmd[MAGIC_START_BYTE_INDEX] = '$';
	m_cmd[MAGIC_STOP_BYTE_INDEX] = '@';
	m_cmd[MSG_TYPE_INDEX] = 'A';

}

int SerialCom::serial_write(char* cmd, int cmd_size)
{
	return write(m_dev, cmd, cmd_size);
}

int SerialCom::serial_read()
{
	
	return read(m_dev, &m_read_buffer, sizeof(m_read_buffer));
}

void SerialCom::callback(const vortex_msgs::Pwm& msg)
{
	//ROS_INFO("I heard: ");
	//ROS_INFO("%d ", msg.positive_width_us[0]);

	std::cout << "I heard: " 
	  		  << msg.positive_width_us[0] 
	  		  << std::endl;

	std::cout << "Command: "
			  << m_cmd[MAGIC_START_BYTE_INDEX] 
		 	  << m_cmd[MSG_TYPE_INDEX];

	uint8_t char1, char2;
	char1 = (uint8_t)(msg.positive_width_us[1]  >> 8);    // get the high 8 bits
	char2 = (uint8_t)(msg.positive_width_us[1]  & 0xFF);  // isolate the low 8 bits

    int i;
	for (i = MSG_PAYLOAD_START_INDEX; 
		 i <= MSG_PAYLOAD_STOP_INDEX; 
		 i++)
	{
		if ((i % 2) == 0)
			m_cmd[i] = (char)char1;
		else
			m_cmd[i] = (char)char2;

		printf(" %d ", m_cmd[i]);
	}

	std::cout << m_cmd[MAGIC_STOP_BYTE_INDEX] << std::endl;

	int n = serial_write(&m_cmd[0], MAX_MSG_SIZE);

	/* Error Handling */
	if (n < 0)
	{
	    std::cout << "Error writing: " 
	              << strerror(errno) 
	              << std::endl;
	}



}

SerialCom::~SerialCom()
{
	close(m_dev);
}