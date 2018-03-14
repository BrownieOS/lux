
DefinitionBlock("acpi.aml", "DSDT", 1, "TEST!", "", 1)
{
	Name(KOT, 123456789)
	Name(TOT, Package(4)
	{
		5,7,8,9
	})

	Method(TEST, 0, NotSerialized)
	{
		TST1()
		Return (7)
	}

	Method(TST1, 0, NotSerialized)
	{
		Local1 = 8
		Local2 = 18
		Local5 = Local2
		Return(Local5)
	}
}


