#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xa78af5f3, "ioread32" },
	{ 0x4a453f53, "iowrite32" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x92997ed8, "_printk" },
	{ 0xce502b22, "pci_get_device" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x449ad0a7, "memcmp" },
	{ 0xdae1836f, "kmalloc_caches" },
	{ 0xad01eb0f, "kmem_cache_alloc_trace" },
	{ 0xde80cd09, "ioremap" },
	{ 0x8f2ac704, "gpiochip_add_data_with_key" },
	{ 0xedc03953, "iounmap" },
	{ 0x37a0cba, "kfree" },
	{ 0x9166fada, "strncpy" },
	{ 0xf1d087d0, "i2c_bit_add_numbered_bus" },
	{ 0xacfbcdfc, "gpiochip_remove" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x47ae5f1a, "i2c_del_adapter" },
	{ 0x5d48dd6, "module_layout" },
};

MODULE_INFO(depends, "i2c-algo-bit");


MODULE_INFO(srcversion, "4087B7C982A9202A8375220");
