#pragma once
#ifndef OFFSETS_H_INCLUDED
#define OFFSETS_H_INCLUDED

#include <windows.h>
#include <cstdint>

//
// offsets.h
// Centralne deklaracje offsetów dla ró¿nych buildów FiveM.
// Deklaracje tutaj; definicje w offsets.cpp (jedna instancja -> brak ODR).
//
// Ka¿dy offset reprezentowany jako uintptr_t (bezpieczny dla wskaŸników).
//

namespace offset {

	// Flagi detekcji wersji (dodane dla wszystkich wspieranych buildów).
	extern bool detectv1604;
	extern bool detectv2060;
	extern bool detectv2189;
	extern bool detectv2372;
	extern bool detectv2545;
	extern bool detectv2612;
	extern bool detectv2699;
	extern bool detectv2802;
	extern bool detectv2944;
	extern bool detectv3095;

	// Grupa offsetów dla jednej wersji -> u³atwia utrzymanie i kopiowanie.
	struct VersionOffsets {
		uintptr_t ReplayInterface_ptr;
		uintptr_t world_ptr;
		uintptr_t viewport_ptr;
	};

	// Deklaracje obiektów z offsetami (definicje w offsets.cpp)
	extern VersionOffsets b1604;
	extern VersionOffsets b2060;
	extern VersionOffsets b2189;
	extern VersionOffsets b2372;
	extern VersionOffsets b2545;
	extern VersionOffsets b2612;
	extern VersionOffsets b2699;
	extern VersionOffsets b2802;
	extern VersionOffsets b2944;
	extern VersionOffsets b3095;

	// Krótka dokumentacja:
	// - ReplayInterface_ptr: wskaŸnik do interfejsu Replay (u¿ywany do enumeracji Pedów)
	// - world_ptr: wskaŸnik do œwiata / globalnego obiektu gry
	// - viewport_ptr: wskaŸnik do struktury viewport (do konwersji/world->screen)
	//
	// U¿ycie:
	//   uintptr_t off_world = offset::b1604.world_ptr;
	//   SetLocalPlayerSafe(read_mem<uintptr_t>(base + off_world));
	//
}

#endif // OFFSETS_H_INCLUDED