#include <sstream>
#include "libfreenect.hpp"
#include "Driver/OniDriverAPI.h"
//#include "KinectDriver.h"
#include "FreenectDeviceNI.h"
#include "XnHash.h"


using namespace oni::driver;

static const char VENDOR_VAL[] = "Microsoft";
static const char NAME_VAL[] = "Kinect";


class FreenectDriver : public DriverBase, private Freenect::Freenect
{
private:
	//Freenect::Freenect freenect;
	// from Freenect::Freenect - freenect_context* m_ctx
	xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*> devices;

public:
	FreenectDriver(OniDriverServices* pDriverServices) : DriverBase(pDriverServices)
	{
		freenect_set_log_level(m_ctx, FREENECT_LOG_NOTICE);
		// MOTOR doesn't work with k4w branch; todo: fix it
		//freenect_select_subdevices(m_ctx, static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
		freenect_select_subdevices(m_ctx, static_cast<freenect_device_flags>(FREENECT_DEVICE_CAMERA));
	}
	~FreenectDriver()
	{}
	
	OniStatus initialize(DeviceConnectedCallback connectedCallback, DeviceDisconnectedCallback disconnectedCallback, DeviceStateChangedCallback deviceStateChangedCallback, void* pCookie)
	{
		DriverBase::initialize(connectedCallback, disconnectedCallback, deviceStateChangedCallback, pCookie);
		for (int i = 1; i <= Freenect::deviceCount(); i++)
		{
			OniDeviceInfo* pInfo = XN_NEW(OniDeviceInfo);
			std::ostringstream uri;
			uri << "freenect://" << i;
			xnOSStrCopy(pInfo->uri, uri.str().c_str(), ONI_MAX_STR);
			xnOSStrCopy(pInfo->vendor, VENDOR_VAL, ONI_MAX_STR);
			xnOSStrCopy(pInfo->name, NAME_VAL, ONI_MAX_STR);
			devices[pInfo] = NULL;
			deviceConnected(pInfo);
			deviceStateChanged(pInfo, 0);	
		}
		return ONI_STATUS_OK;
	}	
	
	DeviceBase* deviceOpen(const char* uri)
	{
		for (xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*>::Iterator iter = devices.Begin(); iter != devices.End(); ++iter)
		{
			if (xnOSStrCmp(iter->Key()->uri, uri) == 0)
			{
				// found
				if (iter->Value() != NULL)
				{
					// already using
					return iter->Value();
				}
				else
				{
					int id;
					std::istringstream(iter->Key()->uri) >> id;
					FreenectDeviceNI * device = &createDevice<FreenectDeviceNI>(id);
					iter->Value() = device;
					return device;
				}
			}
		}
		
		getServices().errorLoggerAppend("Looking for '%s'", uri);
		return NULL;	
	}
	
	void deviceClose(DeviceBase* pDevice)
	{
		for (xnl::Hash<OniDeviceInfo*, oni::driver::DeviceBase*>::Iterator iter = devices.Begin(); iter != devices.End(); ++iter)
		{
			if (iter->Value() == pDevice)
			{
				iter->Value() = NULL;
				int id;
				std::istringstream(iter->Key()->uri) >> id;
				Freenect::deleteDevice(id);
				return;
			}
		}
		
		// not our device?!
		XN_ASSERT(FALSE);
	}
	
	void shutdown() { }
	
	OniStatus tryDevice(const char* uri)
	{
		DeviceBase* device = deviceOpen(uri);
		if (device == NULL)
			return ONI_STATUS_ERROR;
		deviceClose(device);
		
		return ONI_STATUS_OK;
	}


	/* unimplemented from DriverBase
	virtual void* enableFrameSync(oni::driver::StreamBase** pStreams, int streamCount);
	virtual void disableFrameSync(void* frameSyncGroup);
	*/
};


ONI_EXPORT_DRIVER(FreenectDriver);