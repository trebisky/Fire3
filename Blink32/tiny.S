	.global blink_main
	.global start

start:
	bl		blink_main
	b		.

	// leave space for header info.
	.skip		0x54, 0

// THE END
