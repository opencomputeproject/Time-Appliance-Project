#include "fpga.h"


void onFpgaProgramN(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t value = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Fpga ProgramN no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("Fpga ProgramN needs 1 argument!");
    return;
  } 

  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 1), &value ) ) {
    Serial.println("Failed to parse first argument for uint8_t");
    return;
  }
  if ( value ) {
    Serial.println("FPGA ProgramN Setting high!");
    wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
    wwvb_digital_write(FPGA_PROGRAMN, 1);
  } else {
    Serial.println("FPGA ProgramN setting low!");
    wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
    wwvb_digital_write(FPGA_PROGRAMN, 0);
  }
}


static Node fpga_programn_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "manual_programn",
    "Manually control the programn fpga pin",
    true,
    nullptr,
    onFpgaProgramN
  }
};



/********************* Boiler plate CLI stuff ***********/

void fpga_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration


static Node * fpga_files[] = { &fpga_programn_node};

static Node fpga_dir = {
    .name = "fpga",
    .type = MY_DIRECTORY,
    .cliBinding = {"fpga",
          "fpga mode",
          true,
          nullptr,
          fpga_dir_operation},
    .parent = 0,
    .children = fpga_files,
    .num_children = sizeof(fpga_files) / sizeof(fpga_files[0])
};

void fpga_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&fpga_dir);
}

// Initialize function to set the parent pointers if needed
void fpga_fs_init() {
  for (int i = 0; i < fpga_dir.num_children; i++) {
    fpga_files[i]->parent = &fpga_dir;
  }
  add_root_filesystem(&fpga_dir);
}







void init_fpga_cli()
{
  wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
  wwvb_digital_write(FPGA_PROGRAMN, 0); // hold FPGA in reset by default
  fpga_fs_init();
}