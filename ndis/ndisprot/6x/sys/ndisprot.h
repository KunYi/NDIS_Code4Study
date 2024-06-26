/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndisprot.h

Abstract:

    Data structures, defines and function prototypes for NDISPROT.

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef __NDISPROT__H
#define __NDISPROT__H

#pragma warning(disable:28930) // Unused assignment of pointer, by design in samples
#pragma warning(disable:28931) // Unused assignment of variable, by design in samples

//
// Update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version.
// Also make sure that VER_FILEVERSION specified in the .RC file also
// matches with the driver version.
//
// Let's say we're version 4.2.
//
#define MAJOR_DRIVER_VERSION           0x04
#define MINOR_DRIVER_VERISON           0x02


//
// Define the NDIS protocol interface version that this driver targets.
//
#if defined(NDIS60)
#  define NDIS_PROT_MAJOR_VERSION             6
#  define NDIS_PROT_MINOR_VERSION             0
#elif defined(NDIS630)
#  define NDIS_PROT_MAJOR_VERSION             6
#  define NDIS_PROT_MINOR_VERSION            30
#else
#  error Unsupported NDIS version
#endif


#define NT_DEVICE_NAME          L"\\Device\\Ndisprot"
#define DOS_DEVICE_NAME         L"\\Global??\\Ndisprot"


//
//  Abstract types
//
typedef NDIS_EVENT              NPROT_EVENT, *PNPROT_EVENT;

#define MAX_MULTICAST_ADDRESS   0x20

#define NPROT_MAC_ADDR_LEN            6


typedef enum _NDISPROT_OPEN_STATE{
    NdisprotInitializing,
    NdisprotRunning,
    NdisprotPausing,
    NdisprotPaused,
    NdisprotRestarting,
    NdisprotClosing
} NDISPROT_OPEN_STATE;
//
//  The Open Context represents an open of our device object.
//  We allocate this on processing a BindAdapter from NDIS,
//  and free it when all references (see below) to it are gone.
//
//  Binding/unbinding to an NDIS device:
//
//  On processing a BindAdapter call from NDIS, we set up a binding
//  to the specified NDIS device (miniport). This binding is
//  torn down when NDIS asks us to Unbind by calling
//  our UnbindAdapter handler.
//
//  Receiving data:
//
//  While an NDIS binding exists, read IRPs are queued on this
//  structure, to be processed when packets are received.
//  If data arrives in the absence of a pended read IRP, we
//  queue it, to the extent of one packet, i.e. we save the
//  contents of the latest packet received. We fail read IRPs
//  received when no NDIS binding exists (or is in the process
//  of being torn down).
//
//  Sending data:
//
//  Write IRPs are used to send data. Each write IRP maps to
//  a single NDIS packet. Packet send-completion is mapped to
//  write IRP completion. We use NDIS 5.1 CancelSend to support
//  write IRP cancellation. Write IRPs that arrive when we don't
//  have an active NDIS binding are failed.
//
//  Reference count:
//
//  The following are long-lived references:
//  OPEN_DEVICE ioctl (goes away on processing a Close IRP)
//  Pended read IRPs
//  Queued received packets
//  Uncompleted write IRPs (outstanding sends)
//  Existence of NDIS binding
//
typedef struct _NDISPROT_OPEN_CONTEXT
{
    LIST_ENTRY              Link;           // Link into global list
    ULONG                   Flags;          // State information
    ULONG                   RefCount;
    NPROT_LOCK               Lock;

    PFILE_OBJECT            pFileObject;    // Set on OPEN_DEVICE

    NDIS_HANDLE             BindingHandle;
    NDIS_HANDLE             SendNetBufferListPool;  
    // let every net buffer list contain one net buffer(don't know how many net buffers can be include in one list.
    NDIS_HANDLE             RecvNetBufferListPool;
    
    ULONG                   MacOptions;
    ULONG                   MaxFrameSize;
    ULONG                   DataBackFillSize;
    ULONG                   ContextBackFillSize;

    LIST_ENTRY              PendedWrites;   // pended Write IRPs
    ULONG                   PendedSendCount;

    LIST_ENTRY              PendedReads;    // pended Read IRPs
    ULONG                   PendedReadCount;
    LIST_ENTRY              RecvNetBufListQueue;
    ULONG                   RecvNetBufListCount;

    NET_DEVICE_POWER_STATE  PowerState;
    NDIS_EVENT              PoweredUpEvent; // signalled iff PowerState is D0
    NDIS_STRING             DeviceName;     // used in NdisOpenAdapter
    NDIS_STRING             DeviceDescr;	// friendly name

    NDIS_STATUS             BindStatus;     // for Open/CloseAdapter
    NPROT_EVENT             BindEvent;      // for Open/CloseAdapter

    ULONG                   oc_sig;         // Signature for sanity
    NDISPROT_OPEN_STATE     State;
    PNPROT_EVENT            ClosingEvent;      
    UCHAR                   CurrentAddress[NPROT_MAC_ADDR_LEN];
    UCHAR                   MCastAddress[MAX_MULTICAST_ADDRESS][NPROT_MAC_ADDR_LEN];
} NDISPROT_OPEN_CONTEXT, *PNDISPROT_OPEN_CONTEXT;

    
#define oc_signature        'OiuN'

//
//  Definitions for Flags above.
//
#define NPROTO_BIND_IDLE             0x00000000
#define NPROTO_BIND_OPENING          0x00000001
#define NPROTO_BIND_FAILED           0x00000002
#define NPROTO_BIND_ACTIVE           0x00000004
#define NPROTO_BIND_CLOSING          0x00000008
#define NPROTO_BIND_FLAGS            0x0000000F  // State of the binding

#define NPROTO_OPEN_IDLE             0x00000000
#define NPROTO_OPEN_ACTIVE           0x00000010
#define NPROTO_OPEN_FLAGS            0x000000F0  // State of the I/O open

#define NPROTO_RESET_IN_PROGRESS     0x00000100
#define NPROTO_NOT_RESETTING         0x00000000
#define NPROTO_RESET_FLAGS           0x00000100

#define NPROTO_MEDIA_CONNECTED       0x00000000
#define NPROTO_MEDIA_DISCONNECTED    0x00000200
#define NPROTO_MEDIA_FLAGS           0x00000200

#define NPROTO_READ_SERVICING        0x00100000  // Is the read service
                                                // routine running?
#define NPROTO_READ_FLAGS            0x00100000

#define NPROTO_UNBIND_RECEIVED       0x10000000  // Seen NDIS Unbind?
#define NPROTO_UNBIND_FLAGS          0x10000000


#define NPROT_ALLOCATED_NBL          0x10000000
#define NPROT_NBL_RETREAT_RECV_RSVD  0x20000000

//
//  Globals:
//
typedef struct _NDISPROT_GLOBALS
{
    PDRIVER_OBJECT          pDriverObject;
    PDEVICE_OBJECT          ControlDeviceObject;
    NDIS_HANDLE             NdisProtocolHandle;
    USHORT                  EthType;            // frame type we are interested in
    UCHAR                   PartialCancelId;    // for cancelling sends
    ULONG                   LocalCancelId;
    LIST_ENTRY              OpenList;           // of OPEN_CONTEXT structures
    NPROT_LOCK              GlobalLock;         // to protect the above
    NPROT_EVENT             BindsComplete;      // have we seen NetEventBindsComplete?
} NDISPROT_GLOBALS, *PNDISPROT_GLOBALS;


//
//  The following are arranged in the way a little-endian processor
//  would read 2 bytes off the wire.
//
#define NPROT_ETH_TYPE               0x8e88
#define NPROT_8021P_TAG_TYPE         0x0081

//
//  NDIS Request context structure
//
typedef struct _NDISPROT_REQUEST
{
    NDIS_OID_REQUEST         Request;
    NPROT_EVENT              ReqEvent;
    ULONG                    Status;

} NDISPROT_REQUEST, *PNDISPROT_REQUEST;


#define NPROTO_PACKET_FILTER  (NDIS_PACKET_TYPE_DIRECTED|    \
                              NDIS_PACKET_TYPE_MULTICAST|   \
                              NDIS_PACKET_TYPE_BROADCAST)

//
//  Send packet pool bounds
//
/*
#define MIN_SEND_PACKET_POOL_SIZE    20
*/
#define MAX_SEND_PACKET_POOL_SIZE    400


//
//  ProtocolReserved in sent packets. We save a pointer to the IRP
//  that generated the send.
//
//  The RefCount is used to determine when to free the packet back
//  to its pool. It is used to synchronize between a thread completing
//  a send and a thread attempting to cancel a send.
//
typedef struct _NPROT_SEND_NETBUFLIST_RSVD
{
    PIRP                    pIrp;
    ULONG                   RefCount;

} NPROT_SEND_NETBUFLIST_RSVD, *PNPROT_SEND_NETBUFLIST_RSVD;
//
//  Receive packet pool bounds
//
#define MIN_RECV_PACKET_POOL_SIZE    4
#define MAX_RECV_PACKET_POOL_SIZE    20

//
//  Max receive packets we allow to be queued up
//
#define MAX_RECV_QUEUE_SIZE          4

//
//  ProtocolReserved in received packets: we link these
//  packets up in a queue waiting for Read IRPs.
//
typedef struct _NPROT_RECV_NBL_RSVD
{
    LIST_ENTRY              Link;
    PNET_BUFFER_LIST        pNetBufferList;    // used if we had to partial-map

} NPROT_RECV_NBL_RSVD, *PNPROT_RECV_NBL_RSVD;


#include <pshpack1.h>

typedef struct _NDISPROT_ETH_HEADER
{
    UCHAR       DstAddr[NPROT_MAC_ADDR_LEN];
    UCHAR       SrcAddr[NPROT_MAC_ADDR_LEN];
    USHORT      EthType;

} NDISPROT_ETH_HEADER;

typedef struct _NDISPROT_ETH_HEADER UNALIGNED * PNDISPROT_ETH_HEADER;

#include <poppack.h>


extern NDISPROT_GLOBALS      Globals;


#define NPROT_ALLOC_TAG      'oiuN'


//
//  Prototypes.
//

DRIVER_INITIALIZE  DriverEntry;
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriverObject,
    IN PUNICODE_STRING  pRegistryPath
    );

DRIVER_UNLOAD NdisprotUnload;
VOID
NdisprotUnload(
    IN PDRIVER_OBJECT   pDriverObject
    );

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH NdisprotOpen;

NTSTATUS
NdisprotOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH NdisprotClose;
NTSTATUS
NdisprotClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

_Dispatch_type_(IRP_MJ_CLEANUP) DRIVER_DISPATCH NdisprotCleanup;
NTSTATUS
NdisprotCleanup(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH NdisprotIoControl;
_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
NdisprotIoControl(
    _In_ IN PDEVICE_OBJECT   pDeviceObject,
    _Inout_  IN PIRP             pIrp
    );

NTSTATUS
ndisprotOpenDevice(
    _In_reads_bytes_(DeviceNameLength) IN PUCHAR     pDeviceName,
    IN ULONG                                    DeviceNameLength,
    IN PFILE_OBJECT                             pFileObject,
    OUT PNDISPROT_OPEN_CONTEXT *                ppOpenContext
    );

VOID
ndisprotRefOpen(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext
    );

VOID
ndisprotDerefOpen(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext
    );

#if DBG
VOID
ndisprotDbgRefOpen(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );

VOID
ndisprotDbgDerefOpen(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    );
#endif // DBG

PROTOCOL_BIND_ADAPTER_EX NdisprotBindAdapter;

PROTOCOL_OPEN_ADAPTER_COMPLETE_EX NdisprotOpenAdapterComplete;

PROTOCOL_UNBIND_ADAPTER_EX NdisprotUnbindAdapter;

PROTOCOL_CLOSE_ADAPTER_COMPLETE_EX NdisprotCloseAdapterComplete;


PROTOCOL_NET_PNP_EVENT NdisprotPnPEventHandler;

VOID
NdisprotProtocolUnloadHandler(
    VOID
    );

NDIS_STATUS
ndisprotCreateBinding(
    IN PNDISPROT_OPEN_CONTEXT                   pOpenContext,
    IN PNDIS_BIND_PARAMETERS                    BindParameters,
    IN NDIS_HANDLE                              BindContext,
    _In_reads_bytes_(BindingInfoLength) IN PUCHAR    pBindingInfo,
    IN ULONG                                    BindingInfoLength
    );

VOID
ndisprotShutdownBinding(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext
    );

VOID
ndisprotFreeBindResources(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext
    );

VOID
ndisprotWaitForPendingIO(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN BOOLEAN                      DoCancelReads
    );

VOID
ndisprotDoProtocolUnload(
    VOID
    );

NDIS_STATUS
ndisprotDoRequest(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN NDIS_PORT_NUMBER             PortNumber,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      pBytesProcessed
    );

NDIS_STATUS
ndisprotValidateOpenAndDoRequest(
    IN PNDISPROT_OPEN_CONTEXT       pOpenContext,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      pBytesProcessed,
    IN BOOLEAN                      bWaitForPowerOn
    );

PROTOCOL_OID_REQUEST_COMPLETE NdisprotRequestComplete;

PROTOCOL_STATUS_EX NdisprotStatus;

NDIS_STATUS
ndisprotQueryBinding(
    _Inout_updates_bytes_to_(OutputLength, *pBytesReturned)
		  PUCHAR					  pBuffer,
    _In_  ULONG                       InputLength,
    _In_  ULONG                       OutputLength,
    _Out_ PULONG                      pBytesReturned
    );

PNDISPROT_OPEN_CONTEXT
ndisprotLookupDevice(
    _In_reads_bytes_(BindingInfoLength) IN PUCHAR    pBindingInfo,
    IN ULONG                                    BindingInfoLength
    );

NDIS_STATUS
ndisprotQueryOidValue(
    IN  PNDISPROT_OPEN_CONTEXT      pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength,
    OUT PULONG                      pBytesWritten
    );

NDIS_STATUS
ndisprotSetOidValue(
    IN  PNDISPROT_OPEN_CONTEXT      pOpenContext,
    OUT PVOID                       pDataBuffer,
    IN  ULONG                       BufferLength
    );

BOOLEAN
ndisprotValidOid(
    IN  NDIS_OID                    Oid
    );

_Dispatch_type_(IRP_MJ_READ) DRIVER_DISPATCH NdisprotRead;
NTSTATUS
NdisprotRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

DRIVER_CANCEL NdisprotCancelRead;
VOID
NdisprotCancelRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

VOID
ndisprotServiceReads(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    );

PROTOCOL_RECEIVE_NET_BUFFER_LISTS NdisprotReceiveNetBufferLists;

VOID
ndisprotQueueReceiveNetBufferList(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNET_BUFFER_LIST              pRcvNetBufList,
    BOOLEAN                          DispatchLevel
    );

_Success_(return != 0)
PNET_BUFFER_LIST
ndisprotAllocateReceiveNetBufferList(
    _In_  PNDISPROT_OPEN_CONTEXT        pOpenContext,
    _In_  UINT                          DataLength,
    _Outptr_result_bytebuffer_(DataLength)
	      PUCHAR *                      ppDataBuffer
    );

VOID
ndisprotFreeReceiveNetBufferList(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNET_BUFFER_LIST              pNetBufferList,
    IN BOOLEAN                       DispatchLevel
    );

VOID
ndisprotCancelPendingReads(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    );

VOID
ndisprotFlushReceiveQueue(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    );

_Dispatch_type_(IRP_MJ_WRITE) DRIVER_DISPATCH  NdisprotWrite;
NTSTATUS
NdisprotWrite(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

DRIVER_CANCEL NdisprotCancelWrite;
VOID
NdisprotCancelWrite(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

PROTOCOL_SEND_NET_BUFFER_LISTS_COMPLETE NdisprotSendComplete;

VOID
ndisprotRestart(
    IN PNDISPROT_OPEN_CONTEXT             pOpenContext,
    IN PNDIS_PROTOCOL_RESTART_PARAMETERS  RestartParameters
    );

#endif // __NDISPROT__H


