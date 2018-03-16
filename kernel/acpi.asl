
DefinitionBlock("acpi.aml", "DSDT", 1, "TEST!", "", 1)
{
	Scope(_SB)
	{
		Device(PCI0)
		{
			Name(_HID, EisaID("PNP0A03"))
			Name(_UID, One)
			Name(_STA, 0x0F)
			Name(_BBN, 0)
		}
	}
}


