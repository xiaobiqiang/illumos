/*
 * Copyright (C) 2007-2014 VMware, Inc. All rights reserved.
 *
 * The contents of this file are subject to the terms of the Common
 * Development and Distribution License (the "License") version 1.0
 * and no later version.  You may not use this file except in
 * compliance with the License.
 *
 * You can obtain a copy of the License at
 *         http://www.opensource.org/licenses/cddl1.php
 *
 * See the License for the specific language governing permissions
 * and limitations under the License.
 */

/*
 * Copyright (c) 2012, 2016 by Delphix. All rights reserved.
 */

#ifndef	_VMXNET3_H_
#define	_VMXNET3_H_

#include <sys/atomic.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/kstat.h>
#include <sys/vtrace.h>
#include <sys/dlpi.h>
#include <sys/strsun.h>
#include <sys/ethernet.h>
#include <sys/vlan.h>
#include <sys/modctl.h>
#include <sys/errno.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/ddi_impldefs.h>
#include <sys/pci.h>
#include <sys/strsubr.h>
#include <sys/pattr.h>
#include <sys/mac.h>
#include <sys/sockio.h>
#include <sys/mac_provider.h>
#include <sys/mac_ether.h>
#include <inet/common.h>
#include <inet/ip.h>
#include <inet/tcp.h>

#include <vmxnet3_defs.h>

typedef struct vmxnet3_dmabuf_t {
	caddr_t		buf;
	uint64_t	bufPA;
	size_t		bufLen;
	ddi_dma_handle_t dmaHandle;
	ddi_acc_handle_t dataHandle;
} vmxnet3_dmabuf_t;

typedef struct vmxnet3_cmdring_t {
	vmxnet3_dmabuf_t dma;
	uint16_t	size;
	uint16_t	next2fill;
	uint16_t	avail;
	uint8_t		gen;
} vmxnet3_cmdring_t;

typedef struct vmxnet3_compring_t {
	vmxnet3_dmabuf_t dma;
	uint16_t	size;
	uint16_t	next2comp;
	uint8_t		gen;
} vmxnet3_compring_t;

typedef struct vmxnet3_metatx_t {
	mblk_t		*mp;
	uint16_t	sopIdx;
	uint16_t	frags;
} vmxnet3_metatx_t;

typedef struct vmxnet3_txqueue_t {
	vmxnet3_cmdring_t cmdRing;
	vmxnet3_compring_t compRing;
	vmxnet3_metatx_t *metaRing;
	Vmxnet3_TxQueueCtrl *sharedCtrl;
} vmxnet3_txqueue_t;

typedef struct vmxnet3_rxbuf_t {
	vmxnet3_dmabuf_t dma;
	mblk_t		*mblk;
	frtn_t		freeCB;
	struct vmxnet3_softc_t *dp;
	struct vmxnet3_rxbuf_t *next;
} vmxnet3_rxbuf_t;

typedef struct vmxnet3_bufdesc_t {
	vmxnet3_rxbuf_t	*rxBuf;
} vmxnet3_bufdesc_t;

typedef struct vmxnet3_rxpool_t {
	vmxnet3_rxbuf_t	*listHead;
	unsigned int	nBufs;
	unsigned int	nBufsLimit;
} vmxnet3_rxpool_t;

typedef struct vmxnet3_rxqueue_t {
	vmxnet3_cmdring_t cmdRing;
	vmxnet3_compring_t compRing;
	vmxnet3_bufdesc_t *bufRing;
	Vmxnet3_RxQueueCtrl *sharedCtrl;
} vmxnet3_rxqueue_t;

typedef struct vmxnet3_softc_t {
	dev_info_t	*dip;
	int		instance;
	mac_handle_t	mac;

	ddi_acc_handle_t pciHandle;
	ddi_acc_handle_t bar0Handle, bar1Handle;
	caddr_t		bar0, bar1;

	boolean_t	devEnabled;
	uint8_t		macaddr[6];
	uint32_t	cur_mtu;
	boolean_t	allow_jumbo;
	link_state_t	linkState;
	uint64_t	linkSpeed;
	vmxnet3_dmabuf_t sharedData;
	vmxnet3_dmabuf_t queueDescs;

	kmutex_t	intrLock;
	int		intrType;
	int		intrMaskMode;
	int		intrCap;
	ddi_intr_handle_t intrHandle;
	ddi_taskq_t	*resetTask;

	kmutex_t	txLock;
	vmxnet3_txqueue_t txQueue;
	ddi_dma_handle_t txDmaHandle;
	boolean_t	txMustResched;

	vmxnet3_rxqueue_t rxQueue;
	kmutex_t	rxPoolLock;
	vmxnet3_rxpool_t rxPool;
	uint32_t	rxMode;
	boolean_t	alloc_ok;

	vmxnet3_dmabuf_t mfTable;
	kstat_t		*devKstats;
	uint32_t	reset_count;
	uint32_t	tx_pullup_needed;
	uint32_t	tx_pullup_failed;
	uint32_t	tx_ring_full;
	uint32_t	tx_error;
	uint32_t	rx_num_bufs;
	uint32_t	rx_alloc_buf;
	uint32_t	rx_alloc_failed;
	uint32_t	rx_pool_empty;
} vmxnet3_softc_t;

typedef struct vmxnet3_kstats_t {
	kstat_named_t	reset_count;
	kstat_named_t	tx_pullup_needed;
	kstat_named_t	tx_ring_full;
	kstat_named_t	rx_alloc_buf;
	kstat_named_t	rx_pool_empty;
	kstat_named_t	rx_num_bufs;
} vmxnet3_kstats_t;

int	vmxnet3_dmaerr2errno(int);
int	vmxnet3_alloc_dma_mem_1(vmxnet3_softc_t *dp, vmxnet3_dmabuf_t *dma,
	    size_t size, boolean_t canSleep);
int	vmxnet3_alloc_dma_mem_128(vmxnet3_softc_t *dp, vmxnet3_dmabuf_t *dma,
	    size_t size, boolean_t canSleep);
int	vmxnet3_alloc_dma_mem_512(vmxnet3_softc_t *dp, vmxnet3_dmabuf_t *dma,
	    size_t size, boolean_t canSleep);
void	vmxnet3_free_dma_mem(vmxnet3_dmabuf_t *dma);
int	vmxnet3_getprop(vmxnet3_softc_t *dp, char *name, int min, int max,
	    int def);

int	vmxnet3_txqueue_init(vmxnet3_softc_t *dp, vmxnet3_txqueue_t *txq);
mblk_t	*vmxnet3_tx(void *data, mblk_t *mps);
boolean_t vmxnet3_tx_complete(vmxnet3_softc_t *dp, vmxnet3_txqueue_t *txq);
void	vmxnet3_txqueue_fini(vmxnet3_softc_t *dp, vmxnet3_txqueue_t *txq);

int	vmxnet3_rxqueue_init(vmxnet3_softc_t *dp, vmxnet3_rxqueue_t *rxq);
mblk_t	*vmxnet3_rx_intr(vmxnet3_softc_t *dp, vmxnet3_rxqueue_t *rxq);
void	vmxnet3_rxqueue_fini(vmxnet3_softc_t *dp, vmxnet3_rxqueue_t *rxq);
void	vmxnet3_log(int level, vmxnet3_softc_t *dp, char *fmt, ...);

extern ddi_device_acc_attr_t vmxnet3_dev_attr;

extern int vmxnet3s_debug;

#define	VMXNET3_MODNAME	"vmxnet3s"
#define	VMXNET3_DRIVER_VERSION_STRING	"1.1.0.0"

/* Logging stuff */
#define	VMXNET3_WARN(Device, ...) \
	dev_err((Device)->dip, CE_WARN, "!" __VA_ARGS__)

#ifdef	DEBUG
#define	VMXNET3_DEBUG(Device, Level, ...) {				\
	if (Level <= vmxnet3s_debug) {					\
		dev_err((Device)->dip, CE_CONT, "?" __VA_ARGS__);	\
	}								\
}
#else
#define	VMXNET3_DEBUG(Device, Level, ...)
#endif

#define	MACADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define	MACADDR_FMT_ARGS(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]

/* Default ring size */
#define	VMXNET3_DEF_TX_RING_SIZE	256
#define	VMXNET3_DEF_RX_RING_SIZE	256

/* Register access helpers */
#define	VMXNET3_BAR0_GET32(Device, Reg) \
	ddi_get32((Device)->bar0Handle, (uint32_t *)((Device)->bar0 + (Reg)))
#define	VMXNET3_BAR0_PUT32(Device, Reg, Value) \
	ddi_put32((Device)->bar0Handle, (uint32_t *)((Device)->bar0 + (Reg)), \
	    (Value))
#define	VMXNET3_BAR1_GET32(Device, Reg) \
	ddi_get32((Device)->bar1Handle, (uint32_t *)((Device)->bar1 + (Reg)))
#define	VMXNET3_BAR1_PUT32(Device, Reg, Value) \
	ddi_put32((Device)->bar1Handle, (uint32_t *)((Device)->bar1 + (Reg)), \
	    (Value))

/* Misc helpers */
#define	VMXNET3_DS(Device) ((Vmxnet3_DriverShared *) (Device)->sharedData.buf)
#define	VMXNET3_TQDESC(Device) \
	((Vmxnet3_TxQueueDesc *) (Device)->queueDescs.buf)
#define	VMXNET3_RQDESC(Device) \
	((Vmxnet3_RxQueueDesc *) ((Device)->queueDescs.buf + \
	    sizeof (Vmxnet3_TxQueueDesc)))

#define	VMXNET3_ADDR_LO(addr) ((uint32_t)(addr))
#define	VMXNET3_ADDR_HI(addr) ((uint32_t)(((uint64_t)(addr)) >> 32))

#define	VMXNET3_GET_DESC(Ring, Idx) \
	(((Vmxnet3_GenericDesc *) (Ring)->dma.buf) + Idx)

/* Rings handling */
#define	VMXNET3_INC_RING_IDX(Ring, Idx) {	\
	(Idx)++;				\
	if ((Idx) == (Ring)->size) {		\
		(Idx) = 0;			\
		(Ring)->gen ^= 1;		\
	}					\
}

#define	VMXNET3_DEC_RING_IDX(Ring, Idx) {	\
	if ((Idx) == 0) {			\
		(Idx) = (Ring)->size;		\
		(Ring)->gen ^= 1;		\
	}					\
	(Idx)--;				\
}

#define	PCI_VENDOR_ID_VMWARE		0x15AD
#define	PCI_DEVICE_ID_VMWARE_VMXNET3	0x07B0

#endif /* _VMXNET3_H_ */
