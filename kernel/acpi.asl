
DefinitionBlock("acpi.aml", "DSDT", 1, "TEST!", "", 1)
{
	Device(PCI0)
	{
		Method(_STA, 0, NotSerialized)
		{
			Local4 = 45
			If(Local4 >= 45)
			{
				Return(0x0F)
			} Else
			{
				Return(0x15)
			}
		}
	}
}


