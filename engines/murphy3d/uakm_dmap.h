#ifndef MURPHY3D_UAKM_DMAP_H
#define MURPHY3D_UAKM_DMAP_H

#include "murphy3d/map.h"

namespace Murphy3d {

class UAKMDMap : public Map {
public:
	bool init() override;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_UAKM_DMAP_H
