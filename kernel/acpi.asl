
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

			Device(BAT0)
			{
				Name(_HID, EisaID("PNP0C0A"))
				Name(_UID, 2)
				Name(_STA, 0x1F)
				Name(_ADR, 0x10000)

				OperationRegion(TST, SystemIO, 0x60, 5)
				Field(TST, ByteAcc, NoLock, Preserve)
				{
					DATA, 8,
					Offset(0x04),
					CMND, 8
				}

				Method(_BIF, 0, NotSerialized)
				{
					Name(BIFP, Package(0xD)
					{
			                    One, 
			                    0x1770, 
			                    0x1770, 
			                    One, 
			                    0x2A30, 
			                    0x0258, 
			                    0xB4, 
			                    0x0108, 
			                    0x0EC4, 
			                    "PABAS0241231", 
			                    "41167", 
			                    "Li-Ion", 
			                    "LENOVO "
					})

					Name(BIF2, Package(0xD)
					{
			                    One, 
			                    0x1770, 
			                    0x1770, 
			                    One, 
			                    0x2A30, 
			                    0x0258, 
			                    0xB4, 
			                    0x0108, 
			                    0x0EC4, 
			                    "Pdeeee1", 
			                    "4ddddd7", 
			                    "Liddddddn", 
			                    "dkoe"
					})

					Local0 = DATA * CMND * 5
					Local0 -= DATA * CMND * 5
		
					If(Local0)
					{
						Return (BIFP)
					} Else
					{
						Return (BIF2)
					}
				}
			}
		}
	}
}


