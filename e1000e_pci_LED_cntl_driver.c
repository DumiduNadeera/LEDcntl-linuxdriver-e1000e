#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/ioport.h>
#ifdef NETIF_F_TSO
#include <net/checksum.h>
#ifdef NETIF_F_TSO6
#include <net/ip6_checksum.h>
#endif
#endif
#include <linux/ethtool.h>
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#include <linux/if_vlan.h>
#endif
#include <linux/prefetch.h>

#include "e1000.h"
#include "kcompat.h"

#ifndef INTEL
#define INTEL 0x8086
#endif

unsigned long hw_address;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dumindu Nadeera Iroshan");
MODULE_DESCRIPTION("Intel e1000e device driver for LED control");

// list of pci_device_id  structures - Pointer to table of device ID's the driver is interested in,
// Used to tell user space which devices this specific driver supports.
// PCI_VDEVICE(VENDOR_ID, DIVICE_ID), board }

static const struct pci_device_id e1000_pci_tbl[] = {
  { PCI_VDEVICE(INTEL, E1000_DEV_ID_82571EB_COPPER), board_82571 },
  { PCI_VDEVICE(INTEL, E1000_DEV_ID_82571EB_FIBER), board_82571 },
  { PCI_VDEVICE(INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER), board_82571 },
  { PCI_VDEVICE(INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER_LP),board_82571 },
  { 0, 0, 0, 0, 0, 0, 0 } /* terminate list */
  };

//export pci_device_id structure to user space 
MODULE_DEVICE_TABLE(pci, e1000_pci_tbl);

static int  e1000_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void e1000_remove(struct pci_dev *pdev);

//PCI Device API Driver - Pointer to a structure describing the driver
static struct pci_driver e1000_driver = {
  .name     = "Dum_e1000e",
  .id_table = e1000_pci_tbl,
  .probe    = e1000_probe,
  .remove   = e1000_remove
};

//This probing function gets called during execution of pci_register_driver()
static int  e1000_probe(struct pci_dev *pdev,const struct pci_device_id *ent)
{
  printk(KERN_ALERT "probe hit ");
}

/*The remove() function gets called whenever a device being handled by this driver is removed (either during
  deregistration of the driver or when it's manually pulled out of a hot-pluggable slot).*/
static void e1000_remove(struct pci_dev *pdev)
{
  printk(KERN_ALERT "remove hit ");   
}

/**
 * e1000_init_module - Driver Registration Routine
 *
 * e1000_init_module is the first routine called when the driver is
 * loaded.Register with the PCI subsystem.
 **/

static int __init e1000_init_module(void)
{  
  
  struct net_device *netdev;
  struct e1000_adapter *adapter;
  struct pci_dev *pdev1 = NULL; 
  int  err, err1, err2;
  unsigned long mmio_start, mmio_len;
  u32 ledctl = 0, ledctl_blink = 0, val;

  /* This function scans the specified subsystem vendor and subsystem device IDs list of PCI devices currently present
     in the system, and if the input arguments match the specified vendor and device IDs, it increments
     the reference count on the struct pci_dev variable found,and returns it to the caller. */

  pdev1 = pci_get_subsys(INTEL,E1000_DEV_ID_82571EB_COPPER,PCI_ANY_ID,PCI_ANY_ID,NULL);

  //initialize network device
  netdev = alloc_etherdev(sizeof(struct e1000_adapter));
  if (!netdev){
    printk(KERN_ALERT "alloc_etherdev error");
  }
  
  //pci_set_drvdata(pdev1, netdev); //Set private driver data pointer for a pci_dev

  //retrieves the pointer to the driver-private area of a network device structure.
  adapter = netdev_priv(netdev);

  //returns bus start address for a given PCI region
  mmio_start = pci_resource_start (pdev1, 0); 
  //returns the byte length of a PCI region
  mmio_len   = pci_resource_len (pdev1, 0);   

  //allocate or free a contiguous virtual address space. ioremap accesses physical memory through virtual addresses
  adapter->hw.hw_addr = ioremap(mmio_start, mmio_len);
  if (!adapter->hw.hw_addr){    
    printk(KERN_ALERT "err_ioremap"); 
  } 

  //assign base address to globle hw_address variable  
  hw_address = (unsigned long) adapter->hw.hw_addr;

  /*always blink LED0 */
  ledctl_blink = E1000_LEDCTL_LED0_BLINK | (E1000_LEDCTL_MODE_LED_ON << E1000_LEDCTL_LED0_MODE_SHIFT); 
  //E1000_PHY_LED0_MODE_MASK

/*

LED Control
LEDCTL Reg (00E00h; RW)
 LED Control Bit Description
 Field              Bit   Initial Value    Description 
 LED0_MODE          3:0       0010b         LED0/LINK# Mode. This field specifies the control source for the LED0 output.
                                            An initial value of 0010b selects LINK_UP# indication.
 Reserved           4            0b         Reserved. Read-only as 0b. Write as 0b for future compatibility.
 GLOBAL_BLINK_MODE  5            0b         Global Blink Mode. This field specifies the blink mode of all the LEDs. 
                                              0b = Blink at 200 ms on and 200 ms off.
                                              1b = Blink at 83 ms on and 83 ms off.
 LED0_IVRT          6            0b         LED0/LINK# Invert. This field specifies the polarity/ inversion of the LED source prior to output or blink control. 
                                              0b = Do not invert LED source.
                                              1b = Invert LED source.
 LED0_BLINK         7            0b         LED0/LINK# Blink. This field specifies whether to apply blink logic to the (possibly inverted) LED control source prior to the LED output.
                                              0b = Do not blink asserted LED output.
                                              1b = Blink asserted LED output.
 LED1_MODE         11:8       0011b         LED1/ACTIVITY# Mode. This field specifies the control source for the LED1 output. 
                                            An initial value of 0011b selects ACTIVITY# indication.
 Reserved           12           0b         Reserved. Read-only as 0b. Write as 0b for future compatibility.
 LED1_BLINK_MODE    13           0b         LED1 Blink Mode. This field needs to be configured with the same value as GLOBAL_BLINK_MODE as it specifies the blink mode of the LED.
                                              0b = Blink at 200 ms on and 200ms off.
                                              1b = Blink at 83 ms on and 83 ms off. 
 LED1_IVRT          14           0b         LED1/ACTIVITY# Invert.
 LED1_BLINK         15           1b         LED1/ACTIVITY# Blink.
 LED2_MODE       19:16        0110b         LED2/LINK100# Mode. This field specifies the control source for the LED2 output. An initial value of 0011b selects LINK100# indication.
 Reserved           20          00b         Reserved. Read-only as 0b. Write as 0b for future compatibility.
 LED2_BLINK_MODE    21           0b         LED2/LINK100# Blink Mode. This field needs to be configured with the same value as GLOBAL_BLINK_MODE as it specifies the blink mode of the LED.
                                              0b = Blink at 200 ms on and 200 ms off.
                                              1b = Blink at 83 ms on and 83 ms off. 
 LED2_IVRT          22           0b         LED2/LINK100# Invert. 
 LED2_BLINK         23           0b         LED2/LINK100# Blink.
 LED3_MODE       27:240        111b         LED3/LINK1000# Mode. This field specifies the control source for the LED3 output. An initial value of 0111b selects LINK1000# indication.This is a reserved bit for the 82573E/82573V/82573L.
 Reserved           28           0b         Reserved. Read-only as 0b. Write as 0b for future compatibility.
 LED3_BLINK_MODE    29           0b         LED3/LINK1000# Blink Mode. This field needs to be configured with the same value as GLOBAL_BLINK_MODE as it specifies the blink mode of the LED.
                                              0b = Blink at 200 ms on and 200ms off.
                                              1b = Blink at 83 ms on and 83 ms off.
                                              This is a reserved bit for the 82573E/82573V/82573L.
 LED3_IVRT          30           0b         LED3/LINK1000# Invert.This is a reserved bit for the 82573E/82573V/82573L.
 LED3_BLINK         31           0b         LED3/LINK1000# Blink.This is a reserved bit for the 82573E/82573V/82573L.


 Mode Encodings for LED Outputs

  Mode       Pneumonic         State / Event Indicated

  0000       bLINK_10/1000    Asserted when either 10 or 1000 Mb/s link is established and maintained.
  0001       bLINK_100/1000   Asserted when either 100 or 1000 Mb/s link is established and maintained.
  0010b      LINK_UP          Asserted when any speed link is established and maintained.
  0011b      FILTER_ACTIVITY  Asserted when link is established and packets are being transmitted or received that passed MAC filtering.
  0100b      LINK/ACTIVITY    Asserted when link is established and when there is no transmit or receive activity.
  0101b      LINK_10          Asserted when a 10 Mb/s link is established and maintained.
  0110b      LINK_100         Asserted when a 100 Mb/s link is established and maintained.
  0111b      LINK_1000        Asserted when a 1000 Mb/s link is established and maintained. 
  1001b      FULL_DUPLEX      Asserted when the link is configured for full duplex operation (deasserted in half-duplex).
  1010b      COLLISION        Asserted when a collision is observed.
  1011b      ACTIVITY         Asserted when link is established and packets are being transmitted or received. 
  1100b      BUS_SIZE         Asserted when the Ethernet controller detects a 1 Lane PCIe* connection.
  1101b      PAUSED           Asserted when the Ethernet controller’s transmitter is flow controlled.
  1110b      LED_ON           Always high. Assuming no optional inversion selected, causes output pin high / LED ON for typical LED circuit.
  1111b      LED_OFF          Always low. Assuming no optional inversion selected, causes output pin low / LED OFF for typical LED circuit.

*/

/*LEDCTL 
  LED0                 LED1                     For both LED0 & LED1 

  LED0 OFF = 0xF       LED1 OFF   = 0x0F0000    OFF   = 0x0F000F
  LED0 ON  = 0xE       LED1 ON    = 0x0E0000    ON    = 0x0E000E
  LED0 BLINK = 0x8E    LED1 BLINK = 0x8E0000    BLINK = 0x8E008E  
*/

  //writel write 4 bytes
  writel(0x8E008E, adapter->hw.hw_addr + 0x00E00); //LED0 & LED1 blink
}

module_init(e1000_init_module);

/**
 * e1000_exit_module - Driver Exit Cleanup Routine
 *
 * e1000_exit_module is called just before the driver is removed from memory.
 **/
static void __exit e1000_exit_module(void)
{
  //LED off
  writel(0x0F000F, hw_address + 0x00E00); 

  //undo  mapping  
  iounmap(hw_address);
}

module_exit(e1000_exit_module);



