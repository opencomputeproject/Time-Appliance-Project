#include "my_ethernet.h"






void onEthernetMAC(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int value_count = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("ethernet EEPROM Write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) < 2 ) {
    Serial.println("ethernet EEPROM Write needs at least 2 arguments!");
    return;
  } 

  Serial.println("On ethernet mac!");
}




static Node ethernet_mac_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "mac",
    "Pass MAC to set MAC, pass nothing to print current MAC",
    true,
    nullptr,
    onEthernetMAC
  }
};

/****** Boiler plate kind of code ********/

void ethernet_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * ethernet_files[] = { &ethernet_mac_node };

static Node ethernet_dir = {
    .name = "ethernet",
    .type = MY_DIRECTORY,
    .cliBinding = {"ethernet",
          "ethernet mode",
          true,
          nullptr,
          ethernet_dir_operation},
    .parent = 0,
    .children = ethernet_files,
    .num_children = sizeof(ethernet_files) / sizeof(ethernet_files[0])
};

void ethernet_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&ethernet_dir);
}

// Initialize function to set the parent pointers if needed
void ethernet_fs_init() {
  for (int i = 0; i < ethernet_dir.num_children; i++) {
    ethernet_files[i]->parent = &ethernet_dir;
  }
  add_root_filesystem(&ethernet_dir);
}








void init_ethernet_cli()
{
    // expose ethernet CLI
  ethernet_fs_init();
}