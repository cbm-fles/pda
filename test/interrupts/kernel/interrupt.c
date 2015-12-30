#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/sysfs.h>
#include <linux/uio_driver.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "intterupt_test_driver"

/** Coment this to test INTX interrupts */
#define MSI_TEST

static DEFINE_PCI_DEVICE_TABLE(device_id) =
{
    {PCI_DEVICE(0x10dc, 0x0001), 0, 0, 0},
    {0, }
};

typedef struct timeval timeval;


struct timeval start_time, end_time;
uint32_t counter         = 0;
void     __iomem   *map  = NULL;
struct   pci_dev   *pdev = NULL;

uint64_t
gettimeofday_diff
(
    timeval time1,
    timeval time2
)
{
    timeval diff;
    diff.tv_sec  = time2.tv_sec  - time1.tv_sec;
    diff.tv_usec = time2.tv_usec - time1.tv_usec;

    return( (diff.tv_sec * 1000) + (diff.tv_usec / 1000) );
}


irqreturn_t irq_handler(int irq, void *dev_id)
{
    u16 value;
    uint8_t *csr;

#ifndef MSI_TEST
    if( !pci_check_and_mask_intx(pdev) )
    { return IRQ_NONE; }
#endif

    counter++;

    csr = (uint8_t*)&value;
    pci_read_config_word(pdev, PCI_COMMAND, &value);
    csr[1] = 1;
    pci_write_config_word(pdev, PCI_COMMAND, value);
    return IRQ_HANDLED;
}

static int
probe
(
    struct pci_dev             *pci_device,
    const struct pci_device_id *id
)
{
    printk("Found firedancer test device (yeah!)\n");

    pdev = pci_device;
    printk("IRQ pdev pointer = %p\n", pdev);

    if( 0 != pci_enable_device(pci_device) )
    {
        printk("Unable to enable device!\n");
        return(-1);
    }

    if(!pci_intx_mask_supported(pci_device) )
    {
        printk("Intx interrupting not supported!\n");
        return(-1);
    }

#ifdef MSI_TEST
    if(pci_enable_msi(pci_device))
    {
        printk("Unable to switch to MSI interrupt!\n");
        return(-1);
    }
#endif

    if
    (
        0 != request_irq
        (
            pci_device->irq,
            irq_handler,
            IRQF_SHARED,
            DRIVER_NAME,
            (void *)(irq_handler)
        )
    )
    {
        printk("Unable to request interrupt!\n");
        return(-1);
    }

    if( 0 != pci_request_region(pci_device, 0, DRIVER_NAME) )
    {
        printk("Unable to request BAR!\n");
        return(-1);
    }
    map = pci_iomap(pci_device, 0, 4096);

    do_gettimeofday(&start_time);
    iowrite8(1, map);

    printk("Firedancer test device configured!\n");

    return(0);
}

static void
remove
(
    struct pci_dev *pci_device
)
{
    uint64_t time_millisecond;

    do_gettimeofday(&end_time);
    iowrite8(1, map+1);

    printk("Unregister firedancer test device!\n");

    time_millisecond
        = gettimeofday_diff(start_time, end_time);

    printk("Number of interrupts : %u\n", counter);
    printk("Time (milliseconds) : %llu \n", time_millisecond );
    printk("Interrupts/s : %llu\n", (counter*1000)/time_millisecond );

    pci_disable_device(pci_device);
}



static
struct pci_driver
driver =
{
    .name     = DRIVER_NAME,
    .id_table = device_id,
    .probe    = probe,
    .remove   = remove
};


static int __init
mod_init(void)
{
    return pci_register_driver(&driver);
}

static void __exit
mod_exit(void)
{
    pci_unregister_driver(&driver);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
