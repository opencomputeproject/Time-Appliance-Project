#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/list.h>
#include <linux/etherdevice.h>

#define VENDOR_ID 0x8086 // Replace with your PCIe device vendor ID
#define DEVICE_ID 0x125b // Replace with your PCIe device device ID


/*********** SDP stuff ********/

// I225 registers start at page 361 in user manual pdf


// CTRL register and associated SDP pin bits, controls SDP0 / SDP1
#define CTRL 0x0

#define DIR_IN 0
#define DIR_OUT 1

#define SDP0_IODIR (1<<22)
#define SDP1_IODIR (1<<23)
#define SDP0_DATA (1<<2)
#define SDP1_DATA (1<<3)

// CTRL_EXT register and associated bits, controls SDP2 / SDP3
#define CTRL_EXT 0x18

#define SDP2_IODIR (1<<10)
#define SDP3_IODIR (1<<11)
#define SDP2_DATA (1<<6)
#define SDP3_DATA (1<<7)

// LED1 config register
#define LED_CONFIG 0xe00
#define LED_ALWAYS_ON (0x0)
#define LED_ALWAYS_OFF (0x1)





MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julian St. James");
MODULE_DESCRIPTION("MiniPTM Kernel Module for SDP2/3");


// Register the parameter
//module_param(miniptm_device, charp, S_IRUGO); // charp: character pointer, S_IRUGO: read-only permission


//static void __iomem *mapped_address;
struct my_gpio_chip {
	struct gpio_chip chip;
	struct device_list * my_dev;
};

// Global list to track multiple devices
struct device_list {
    struct list_head list;
    struct pci_dev *pdev;
    void __iomem *mapped_address;
    struct gpio_chip gpio_chip;
    struct i2c_algo_bit_data i2c_bit_data; 
    struct i2c_adapter i2c_adapter;
    // Other device-specific data...
};
static LIST_HEAD(device_list_head);


static const unsigned char known_mac_addresses[][ETH_ALEN] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}, 
    {0x00, 0xa0, 0xc9, 0x00, 0x00, 0x00}, 
    // Add more MAC addresses here...
};




// Configures it as input
static int my_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
    u32 data;
    struct device_list *dev_list;
    void __iomem *mapped_address;

    dev_list = container_of(chip, struct device_list, gpio_chip);
    mapped_address = dev_list->mapped_address;
    //pr_info("GPIO direction_input called. Offset: %u SDP%u\n", offset, offset+2);
    // Implement GPIO direction input setup
    // ...


    data = ioread32(mapped_address+CTRL_EXT);
    
    //pr_info("GPIO direction_input called. Offset: %u SDP%u init=0x%x\n", offset, offset+2, data);
    if ( offset == 0 ) {
	data &= ~SDP2_IODIR;
    } else if ( offset == 1 ) {
	data &= ~SDP3_IODIR;
    }
    iowrite32(data, mapped_address+CTRL_EXT);

    return 0;
}

//  it as output
static int my_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
    u32 data;
    struct device_list *dev_list;
    void __iomem *mapped_address;

    dev_list = container_of(chip, struct device_list, gpio_chip);
    mapped_address = dev_list->mapped_address;
    // Implement GPIO direction output setup
    // ...
    data = ioread32(mapped_address+CTRL_EXT);
    //pr_info("GPIO direction_output called. Offset: %u, Value: %d SDP%u init=0x%x\n", offset, value, offset+2, data);

    // set direction to output
    if ( offset == 0 ) {
	data |= SDP2_IODIR;
    } else if ( offset == 1 ) {
	data |= SDP3_IODIR;
    }
    if ( value ) {
	    if ( offset == 0 ) {
		data |= SDP2_DATA;
	    } else if ( offset == 1 ) {
		data |= SDP3_DATA;
	    }
    } else {
	    if ( offset == 0 ) {
		data &= ~SDP2_DATA;
	    } else if ( offset == 1 ) {
		data &= ~SDP3_DATA;
	    }
    }
    iowrite32(data, mapped_address+CTRL_EXT);
    return 0;
}

// returns value, 0 for low, 1 for high , negative for error
static int my_gpio_get_value(struct gpio_chip *chip, unsigned offset)
{
    u32 data;
    struct device_list *dev_list;
    void __iomem *mapped_address;

    dev_list = container_of(chip, struct device_list, gpio_chip);
    mapped_address = dev_list->mapped_address;
    // Implement GPIO read
    // ...
    data = ioread32(mapped_address+CTRL_EXT);
    //pr_info("GPIO get_value called. Offset: %u SDP%u init=0x%x\n", offset, offset+2, data);
    if ( offset == 0 ) {
	data &= SDP2_DATA;
    } else if ( offset == 1 ) {
	data &= SDP3_DATA;
    }
    if ( data ) {
	return 1;
    }

    return 0;
}

// sets output value
static void my_gpio_set_value(struct gpio_chip *chip, unsigned offset, int value)
{
    u32 data;
    struct device_list *dev_list;
    void __iomem *mapped_address;

    dev_list = container_of(chip, struct device_list, gpio_chip);
    mapped_address = dev_list->mapped_address;
    // Implement GPIO write
    // ...
    data = ioread32(mapped_address+CTRL_EXT);
    //pr_info("GPIO set_value called. Offset: %u, Value: %d SDP%u init=0x%x\n", offset, value, offset+2, data);
    if ( value ) {
	    if ( offset == 0 ) {
		data |= SDP2_DATA;
	    } else if ( offset == 1 ) {
		data |= SDP3_DATA;
	    }
    } else {
	    if ( offset == 0 ) {
		data &= ~SDP2_DATA;
	    } else if ( offset == 1 ) {
		data &= ~SDP3_DATA;
	    }
    }
    iowrite32(data, mapped_address+CTRL_EXT);
}









// SDP2 (0) = SDA, SDP3 (1) = SCL
/*
** Function to read the SCL GPIO
*/
static int MiniPTM_Read_SCL(void *data)
{
    struct device_list *dev_list;
    dev_list = data;

  my_gpio_direction_input(&dev_list->gpio_chip, 1);
  return my_gpio_get_value(&dev_list->gpio_chip, 1);  
}
/*
** Function to read the SDA GPIO
*/
static int MiniPTM_Read_SDA(void *data)
{
    struct device_list *dev_list;
    dev_list = data;
  my_gpio_direction_input(&dev_list->gpio_chip, 0);
  return my_gpio_get_value(&dev_list->gpio_chip, 0);  
}
/*
** Function to set the SCL GPIO
*/
static void MiniPTM_Set_SCL(void *data, int state)
{
    struct device_list *dev_list;
    dev_list = data;
  if ( state ) { // open drain, set to input
	  my_gpio_direction_input(&dev_list->gpio_chip, 1);
  } else {
	  my_gpio_direction_output(&dev_list->gpio_chip, 1, 0);
  }
}
/*
** Function to set the SDA GPIO
*/
static void MiniPTM_Set_SDA(void *data, int state)
{
    struct device_list *dev_list;
    dev_list = data;
  if ( state ) { // open drain, set to input
	  my_gpio_direction_input(&dev_list->gpio_chip, 0);
  } else {
	  my_gpio_direction_output(&dev_list->gpio_chip, 0, 0);
  }
}



static bool is_mac_address_known(const unsigned char *mac_addr) {
    char device_mac[ETH_ALEN * 3]; // 2 chars per byte + ':' separator
    char known_mac[ETH_ALEN * 3];
    int i;

    snprintf(device_mac, sizeof(device_mac), "%pM", mac_addr);

    for (i = 0; i < ARRAY_SIZE(known_mac_addresses); ++i) {
        snprintf(known_mac, sizeof(known_mac), "%pM", known_mac_addresses[i]);
        pr_info("Comparing device MAC: %s with known MAC: %s\n", device_mac, known_mac);

        if (memcmp(mac_addr, known_mac_addresses[i], ETH_ALEN) == 0) {
            pr_info("MAC address match found\n");
            return true;
        }
    }
    pr_info("No MAC address match found\n");
    return false;
}



static int __init miniptm_module_init(void)
{
    //u32 data;
    int ret;
    resource_size_t my_bar;
    resource_size_t my_len;


    struct pci_dev *pdev = NULL;
    struct device_list * dev_list;

    pr_info("Julian's MiniPTM Module start device \n");

    pdev = 0;
    my_bar = 0;
    my_len = 0;


    // Iterate over all PCIe devices with the specified vendor and device ID
    while ((pdev = pci_get_device(VENDOR_ID, DEVICE_ID, pdev)) != NULL) {
	bool mac_match;
	// Check if the device is an Ethernet device
	struct net_device *netdev;



	mac_match = false;
	netdev = pci_get_drvdata(pdev);

	if (!netdev || !is_valid_ether_addr(netdev->dev_addr)) {
		pr_info("Device not ethernet!\n");
		continue; // Not an Ethernet device or invalid MAC address
	}


	// Check if the MAC address matches one of the known addresses
	if (netdev && is_valid_ether_addr(netdev->dev_addr)) {
		if ( is_mac_address_known(netdev->dev_addr) ) {
			mac_match = true;
		}
	}

	if (!mac_match) {
		pr_info("MAC doesn't match!\n");
		continue; // MAC address doesn't match
	}

	// Device matches criteria; allocate and initialize device_list entry
	dev_list = kzalloc(sizeof(*dev_list), GFP_KERNEL);
	if (!dev_list) {
		// Handle allocation failure...
	}

	dev_list->pdev = pdev;


	// map BAR 0
	my_bar = pci_resource_start(pdev, 0);
	my_len = pci_resource_len(pdev, 0);
	pr_info("  BAR 0: Start Address: 0x%llx, Length: 0x%llx\n",
		(unsigned long long)my_bar, (unsigned long long)my_len);


	dev_list->mapped_address = ioremap(my_bar, my_len);

	/*
	// Read data from the fixed address
	data = ioread32(dev_list->mapped_address);
	pr_info("Read data from fixed address: 0x%x\n", data);
	*/

	dev_list->gpio_chip.label = "MiniPTM_GPIO";
	dev_list->gpio_chip.direction_input = my_gpio_direction_input;
	dev_list->gpio_chip.direction_output = my_gpio_direction_output;
	dev_list->gpio_chip.get = my_gpio_get_value;
	dev_list->gpio_chip.set = my_gpio_set_value;
	dev_list->gpio_chip.can_sleep = true;
	dev_list->gpio_chip.base = -1;
	dev_list->gpio_chip.ngpio = 2;

	// Register the GPIO chip with the GPIO subsystem
	ret = gpiochip_add_data(&dev_list->gpio_chip, dev_list);
	if (ret) {
		pr_err("Failed to register GPIO chip: %d\n", ret);
		iounmap(dev_list->mapped_address);
		kfree(dev_list);
		continue;
	}


	dev_list->i2c_bit_data.setsda = MiniPTM_Set_SDA;
	dev_list->i2c_bit_data.setscl = MiniPTM_Set_SCL;
	dev_list->i2c_bit_data.getsda = MiniPTM_Read_SDA;
	dev_list->i2c_bit_data.getscl = MiniPTM_Read_SCL;
	dev_list->i2c_bit_data.udelay = 5;
	dev_list->i2c_bit_data.timeout = 100; // 100ms
	dev_list->i2c_bit_data.data = dev_list; // point to the parent devlist

	dev_list->i2c_adapter.owner = THIS_MODULE;
	dev_list->i2c_adapter.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;

	strncpy(dev_list->i2c_adapter.name , "MiniPTM I2C Adapter", sizeof(dev_list->i2c_adapter.name) - 1);
	// Ensure null termination
	dev_list->i2c_adapter.name[sizeof(dev_list->i2c_adapter.name) - 1] = '\0';

	dev_list->i2c_adapter.algo_data = &dev_list->i2c_bit_data;
	dev_list->i2c_adapter.nr = -1;
	

	ret = i2c_bit_add_numbered_bus(&dev_list->i2c_adapter);
	if (ret < 0) {
		pr_err("Failed to add numbered i2c bus: %d\n", ret);
		// Unregister the GPIO chip
		gpiochip_remove(&dev_list->gpio_chip);
		iounmap(dev_list->mapped_address);
		kfree(dev_list);
		continue;
	}

	// only add to list after all initialization is complete
	INIT_LIST_HEAD(&dev_list->list); 
	list_add(&dev_list->list, &device_list_head);


	// V4 MiniPTM specific changes
	// 1. Disable LED functions for 1G, LED_SPEED_1000# (LED0) / LED_LINK_ACT# (LED2)
	// 	This is a workaround for hardware mistake, next rev should fix
	// 2. LED_SPEED_2500# (LED1) is a board reset, 
	// 	Set it to output and set it high
	// 	Reserved for future software use
	iowrite32( (LED_ALWAYS_OFF << 0) + // LED0
			(LED_ALWAYS_OFF << 8) + // LED1
			(LED_ALWAYS_OFF << 16), // LED2
		dev_list->mapped_address + LED_CONFIG ); // is this off??


	pr_info("Done Insert 1 MiniPTM Basic module\n");
    }
    return 0;
}

static void __exit miniptm_module_exit(void)
{
    struct device_list *dev_list, *tmp;

    list_for_each_entry_safe(dev_list, tmp, &device_list_head, list) {
        // Unmap memory, unregister devices, etc. for dev_list


	i2c_del_adapter(&dev_list->i2c_adapter);	
	// Unregister the GPIO chip
	gpiochip_remove(&dev_list->gpio_chip);
	iounmap(dev_list->mapped_address);

        list_del(&dev_list->list); // Remove from the list
	kfree(dev_list);
    }


    pr_info("MiniPTM Module: Removed\n");
	
}

module_init(miniptm_module_init);
module_exit(miniptm_module_exit);

