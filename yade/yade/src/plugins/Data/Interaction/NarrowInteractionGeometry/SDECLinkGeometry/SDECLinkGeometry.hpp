#ifndef __SDECCPERMANENTLINK_H__
#define __SDECCPERMANENTLINK_H__

#include <vector>

#include "InteractionGeometry.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"

class SDECLinkGeometry : public InteractionGeometry
{
	public : virtual ~SDECLinkGeometry();

	public : Real radius1;
	public : Real radius2;
	public : Vector3r normal;			// new unit normal of the contact plane.

	public : void registerAttributes();

	REGISTER_CLASS_NAME(SDECLinkGeometry);
};

REGISTER_SERIALIZABLE(SDECLinkGeometry,false);

#endif // __SDECCPERMANENTLINK_H__
