#ifndef EBR_H
#define EBR_H

#include <cstring>

#pragma pack(push, 1)
struct EBR {
	char part_mount;
	char part_fit;
	int part_start;
	int part_size;
	int part_next;
	char part_name[16];

	EBR() : part_mount('0'), part_fit('F'), part_start(0), part_size(0), part_next(-1) {
		std::memset(part_name, 0, sizeof(part_name));
	}
};
#pragma pack(pop)

#endif
