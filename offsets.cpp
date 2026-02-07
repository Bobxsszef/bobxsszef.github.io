FiveM External/offsets.cpp
#include "offsets.h"

namespace offset {

	// Detekcje (jedna instancja)
	bool detectv1604 = false;
	bool detectv2060 = false;
	bool detectv2189 = false;
	bool detectv2372 = false;
	bool detectv2545 = false;
	bool detectv2612 = false;
	bool detectv2699 = false;
	bool detectv2802 = false;
	bool detectv2944 = false;
	bool detectv3095 = false;

	// Definicje offsetów — jedna instancja na projekt
	VersionOffsets b1604 = {
		/* ReplayInterface_ptr */ 0x1EFD4C8,
		/* world_ptr */           0x247F840,
		/* viewport_ptr */        0x2087780
	};

	VersionOffsets b2060 = {
		0x1EC3828,
		0x24C8858,
		0x1F6A7E0
	};

	VersionOffsets b2189 = {
		0x1EE18A8,
		0x24E6D90,
		0x1F888C0
	};

	VersionOffsets b2372 = {
		0x1F05208,
		0x252DCD8,
		0x1F9E9F0
	};

	VersionOffsets b2545 = {
		0x1F2E7A8,
		0x25667E8,
		0x1FD6F70
	};

	VersionOffsets b2612 = {
		0x1F77EF0,
		0x2567DB0,
		0x1FD8570
	};

	VersionOffsets b2699 = {
		0x20304C8,
		0x26684D8,
		0x20D8C90
	};

	VersionOffsets b2802 = {
		0x20304C8,
		0x26684D8,
		0x20D8C90
	};

	VersionOffsets b2944 = {
		0x1F42068,
		0x257BEA0,
		0x1FEAAC0
	};

	VersionOffsets b3095 = {
		0x1F42068,
		0x257BEA0,
		0x1FEAAC0
	};

} // namespace offset