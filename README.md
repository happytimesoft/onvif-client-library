# onvif-client-library

1.The onvif client library implements the following ONVIF service:

ONVIF Service	Prefix	Url	version

device	tds	http://www.onvif.org/ver10/device/wsdl	24.06  
event	tev	http://www.onvif.org/ver10/events/wsdl	22.06  
media	trt	http://www.onvif.org/ver10/media/wsdl	21.12  
media 2	tr2	http://www.onvif.org/ver20/media/wsdl	23.06  
ptz	tptz	http://www.onvif.org/ver20/ptz/wsdl	22.12  
image	timg	http://www.onvif.org/ver20/imaging/wsdl	22.06  
analytics	tan	http://www.onvif.org/ver20/analytics/wsdl	22.06  
recording control	trc	http://www.onvif.org/ver10/recording/wsdl	23.06  
search	tse	http://www.onvif.org/ver10/search/wsdl	22.06  
replay	trp	http://www.onvif.org/ver10/replay/wsdl	21.12  
access control	tac	http://www.onvif.org/ver10/accesscontrol/wsdl	21.06  
door control	tdc	http://www.onvif.org/ver10/doorcontrol/wsdl	21.06  
device IO	tmd	http://www.onvif.org/ver10/deviceIO/wsdl	22.06  
thermal	tth	http://www.onvif.org/ver10/thermal/wsdl	22.06  
credential	tcr	http://www.onvif.org/ver10/credential/wsdl	21.06  
access rules	tar	http://www.onvif.org/ver10/accessrules/wsdl	19.06  
schedule	tsc	http://www.onvif.org/ver10/schedule/wsdl	18.12  
receiver	trv	http://www.onvif.org/ver10/receiver/wsdl	21.12  
provisioning	tpv	http://www.onvif.org/ver10/provisioning/wsdl	18.12  
security	tas	http://www.onvif.org/ver10/advancedsecurity/wsdl	25.12  

2.ONVIF Client API
The ONVIF client API in onvif_cln.h file.  
All ONVIF CLIENT APIs comply with ONVIF standard documentations, for details on the interfaces, please refer to the ONVIF standard documents:  
https://www.onvif.org/profiles/specifications/

The API in onvif_api.h file is a simplified version of some commonly used API interfaces.  
ONVIF Application Programmers Guide:   
https://www.onvif.org/wp-content/uploads/2016/12/ONVIF_WG-APG-Application_Programmers_Guide-1.pdf

3.Example  
Onvif client library usage examples please refer OnvifTest.cpp, OnvifTest2.cpp and OnvifTest3.cpp files.  
Onviftest.cpp: Automatically discover onvif devices and perform test steps.  
OnvifTest2.cpp: Manually add devices, get device information, and subscribe events.  
OnvifTest3.cpp: Thread version of OnvifTest2.cpp.

