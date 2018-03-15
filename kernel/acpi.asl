
DefinitionBlock("acpi.aml", "DSDT", 1, "TEST!", "", 1)
{

	OperationRegion(COM, SystemIO, 0x3F8, 1)
	Field(COM, ByteAcc, NoLock, Preserve)
	{
		COM1, 8,
	}

	Method(TEST, 0, NotSerialized)
	{
		Local1 = 255
		Local0 = Zero

		While(Local1)
		{
			COM1 = Local0	// write all chars from 0 to 255 to the serial port
			Local0++
			Local1--
		}
	}

	Method(TST1, 0, NotSerialized)
	{
		Return(8)
	}
}


