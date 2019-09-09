

void __attribute__((section(".entry_function"))) _start(void)
{
	
	//printstr("Hello OS");// Call PMON BIOS printstr to print message "Hello OS!"
	void (*p)(char *string);
	p=0x80011100;

	(*p)("Hello Vincent's OS\n");

	return;
}
