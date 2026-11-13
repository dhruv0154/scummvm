#ifndef MURPHY3D_SILENT_BIC_H
#define MURPHY3D_SILENT_BIC_H

#include "murphy3d/bic.h"

namespace Murphy3d {

class SilentBIC : public BIC {
public:
	SilentBIC(int factor = 1);
	~SilentBIC() override = default;

	bool init(uint8 *pData, int length) override;

protected:
	bool decodeFrame() override;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_SILENT_BIC_H
