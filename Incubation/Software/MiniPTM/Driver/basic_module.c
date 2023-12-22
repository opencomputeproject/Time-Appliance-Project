#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

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





MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julian St. James");
MODULE_DESCRIPTION("MiniPTM Kernel Module for SDP2/3");

// Define a variable to store the parameter value
static char *miniptm_device = "02:00.0";

#define FIXED_ADDRESS 0x6e100000

// Register the parameter
module_param(miniptm_device, charp, S_IRUGO); // charp: character pointer, S_IRUGO: read-only permission


static void __iomem *mapped_address;





// Configures it as input
static int my_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
    u32 data;
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







struct my_gpio_chip {
	struct gpio_chip chip;
};

static struct my_gpio_chip my_gpio_chip = {
    .chip = {
        .label          = "MiniPTM_GPIO",
        .direction_input  = my_gpio_direction_input,
        .direction_output = my_gpio_direction_output,
        .get              = my_gpio_get_value,
        .set              = my_gpio_set_value,
        .ngpio            = 2,  // Number of GPIOs
        .can_sleep        = true, // GPIO functions can sleep

        // Add other GPIO chip configurations as needed
    },
    // Initialize any additional data members for your GPIO controller
};




// SDP2 (0) = SDA, SDP3 (1) = SCL
/*
** Function to read the SCL GPIO
*/
static int MiniPTM_Read_SCL(void *data)
{
  my_gpio_direction_input(&my_gpio_chip.chip, 1);
  return my_gpio_get_value(&my_gpio_chip.chip, 1);  
}
/*
** Function to read the SDA GPIO
*/
static int MiniPTM_Read_SDA(void *data)
{
  my_gpio_direction_input(&my_gpio_chip.chip, 0);
  return my_gpio_get_value(&my_gpio_chip.chip, 0);  
}
/*
** Function to set the SCL GPIO
*/
static void MiniPTM_Set_SCL(void *data, int state)
{
  if ( state ) { // open drain, set to input
	  my_gpio_direction_input(&my_gpio_chip.chip, 1);
  } else {
	  my_gpio_direction_output(&my_gpio_chip.chip, 1, 0);
  }
}
/*
** Function to set the SDA GPIO
*/
static void MiniPTM_Set_SDA(void *data, int state)
{
  if ( state ) { // open drain, set to input
	  my_gpio_direction_input(&my_gpio_chip.chip, 0);
  } else {
	  my_gpio_direction_output(&my_gpio_chip.chip, 0, 0);
  }
}





// I2C bit algorithm Structure
struct i2c_algo_bit_data miniptm_bit_data = {
  .setsda = MiniPTM_Set_SDA,
  .setscl = MiniPTM_Set_SCL,
  .getscl = MiniPTM_Read_SCL,
  .getsda = MiniPTM_Read_SDA,
  .udelay = 5,
  .timeout = 100,       // 100 ms 
};

// I2C adapter Structure
static struct i2c_adapter miniptm_i2c_adapter = {
  .owner      = THIS_MODULE,
  .class      = I2C_CLASS_HWMON | I2C_CLASS_SPD,
  .name       = "MiniPTM I2C Adapter",
  .algo_data  = &miniptm_bit_data,
  .nr         = -1,
};


static int parse_pci_address(const char *pci_address, unsigned int *bus, unsigned int *dev, unsigned int *func)
{
    // Parse the PCI address string in the format "BB:DD.F" (e.g., "02:00.0")

    if (sscanf(pci_address, "%x:%x.%x", bus, dev, func) != 3) {
        pr_err("Invalid PCI address format: %s\n", pci_address);
        return -EINVAL;
    }

    return 0;
}


static int __init basic_module_init(void)
{
    //u32 data;
    int i;
    int ret;
    unsigned int gpio_base; 
    resource_size_t my_bar;
    resource_size_t my_len;
    unsigned int target_bus, target_dev, target_func;


    struct pci_dev *pdev;

    pr_info("Julian's MiniPTM Module start device %s\n", miniptm_device);

    pdev = 0;
    my_bar = 0;
    my_len = 0;


    // Parse the PCI address string
    ret = parse_pci_address(miniptm_device, &target_bus, &target_dev, &target_func);
    if (ret < 0)
        return ret;



    // Iterate over all PCIe devices with the specified vendor and device ID
    while ((pdev = pci_get_device(VENDOR_ID, DEVICE_ID, pdev)) != NULL) {
        if (pdev->bus->number == target_bus &&
            PCI_SLOT(pdev->devfn) == target_dev &&
            PCI_FUNC(pdev->devfn) == target_func) {
            pr_info("Found PCIe Device %04x:%04x at %02x:%02x.%x\n",
                    pdev->vendor, pdev->device,
                    pdev->bus->number, PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));


            // Print BAR addresses
            for (i = 0; i < PCI_STD_RESOURCE_END; ++i) {
                resource_size_t bar_start = pci_resource_start(pdev, i);
                resource_size_t bar_len = pci_resource_len(pdev, i);

                if (bar_start) {
                    pr_info("  BAR %d: Start Address: 0x%llx, Length: 0x%llx\n",
                           i, (unsigned long long)bar_start, (unsigned long long)bar_len);
		    if ( i == 0 ) {
			    my_bar = bar_start;
			    my_len = bar_len;
		    }
                }
            }

            break; // If you only need the first matching device, you can break out of the loop
        }
    }
    if ( !pdev ) {
	pr_info(" MiniPTM module didn't find device %s,end\n", miniptm_device);
	return -ENODEV;
    }
    if ( pdev ) pci_dev_put(pdev); // Release the reference obtained by pci_get_device


    mapped_address = ioremap(my_bar, my_len);

    if (!mapped_address) {
        pr_err("Failed to map fixed address\n");
        return -ENOMEM;
    }

    /*
    // Read data from the fixed address
    data = ioread32(mapped_address);
    pr_info("Read data from fixed address: 0x%x\n", data);
    */



    // Register the GPIO chip with the GPIO subsystem
    ret = gpiochip_add_data(&my_gpio_chip.chip, &my_gpio_chip);
    if (ret < 0) {
        pr_err("Failed to register GPIO chip: %d\n", ret);
        return ret;
    }


    gpio_base = my_gpio_chip.chip.base;




    ret = i2c_bit_add_numbered_bus(&miniptm_i2c_adapter);
    if (ret < 0) {
        pr_err("Failed to add numbered i2c bus: %d\n", ret);
	if (mapped_address)
		iounmap(mapped_address);
	// Unregister the GPIO chip
	gpiochip_remove(&my_gpio_chip.chip);
        return ret;
    }
    pr_info("Done insert MiniPTM Basic module\n");

    return 0;
}

static void __exit basic_module_exit(void)
{
    if (mapped_address)
        iounmap(mapped_address);


    // Unregister the GPIO chip
    gpiochip_remove(&my_gpio_chip.chip);

    pr_info("Basic Module: Removed\n");
	
}

module_init(basic_module_init);
module_exit(basic_module_exit);

