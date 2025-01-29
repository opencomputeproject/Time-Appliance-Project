
#include "lora_cli.h"




void onLoRACLITX(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Sending pre-canned LoRA TX packet");
}

void onLoRA_CLI_Tx_IQ(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t print_iq_sample_count = 1000;
  if (embeddedCliGetTokenCount(args) == 1) {
    print_iq_sample_count = atoi( embeddedCliGetToken(args, 1) );
  } 
  sprintf(print_buffer, "Sending pre-canned LoRA TX packet and printing %d samples\r\n", print_iq_sample_count);
  Serial.print(print_buffer);
}
/***************** Boiler plate CLI stuff *********/

static Node lora_cli_tx_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "tx",
    "Transmit one predefined packet",
    true,
    nullptr,
    onLoRACLITX
  }
};

static Node lora_cli_tx_iq_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "txiq",
    "Transmit one predefined packet and print latest IQ samples, default count 1000, pass value for different",
    true,
    nullptr,
    onLoRA_CLI_Tx_IQ
  }
};



void lora_cli_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * lora_cli_files[] = { &lora_cli_tx_node, &lora_cli_tx_iq_node };

static Node lora_cli_dir = {
    .name = "lora_cli",
    .type = MY_DIRECTORY,
    .cliBinding = {"lora_cli",
          "lora_cli mode",
          true,
          nullptr,
          lora_cli_dir_operation},
    .parent = 0,
    .children = lora_cli_files,
    .num_children = sizeof(lora_cli_files) / sizeof(lora_cli_files[0])
};

void lora_cli_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&lora_cli_dir);
}

// Initialize function to set the parent pointers if needed
void lora_cli_fs_init() {
  for (int i = 0; i < lora_cli_dir.num_children; i++) {
    lora_cli_files[i]->parent = &lora_cli_dir;
  }
  add_root_filesystem(&lora_cli_dir);
}




void init_lora_cli()
{
  Serial.println("Init LoRA CLI");

    // expose lora_cli CLI
  lora_cli_fs_init();
  Serial.println("LoRA CLI initialized!");
}