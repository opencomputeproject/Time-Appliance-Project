

#include "menu_cli.h"



Node *current;
Node * want_new_dir = 0;
//static rtos::Mutex want_new_dir_mutex;

// some forward declarations
Node *change_directory(Node *current, const char *name);
void restart_cli();
void addMenuCLI_current_directory();
void addMenuCLI_standard_commands();

// 164 bytes is minimum size for this params on Arduino Nano
#define CLI_BUFFER_SIZE 8192
#define CLI_RX_BUFFER_SIZE 1024
#define CLI_CMD_BUFFER_SIZE 128
#define CLI_HISTORY_SIZE 1024
#define CLI_BINDING_COUNT 100

EmbeddedCli *cli;

CLI_UINT cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

void onCommand(EmbeddedCli *embeddedCli, CliCommand *command);

void writeChar(EmbeddedCli *embeddedCli, char c);

void onHello(EmbeddedCli *cli, char *args, void *context);

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

rtos::Thread cliThread;


void cli_loop()
{
  while (1) {
    // provide all chars to cli
    if ( want_new_dir != 0 && want_new_dir != current ) {
      current = want_new_dir;
      want_new_dir = 0;
      restart_cli();
      addMenuCLI_standard_commands();
      addMenuCLI_current_directory();
      Serial.println("");
    }
    if ( !Serial ) {
      rtos::ThisThread::sleep_for(50);
    }
    while (Serial.available() > 0) {
        embeddedCliReceiveChar(cli, Serial.read());
    }
    embeddedCliProcess(cli);
    rtos::ThisThread::sleep_for(50); // 50ms , maybe adjust if cli not responsive
  }
}

char prompt[32];
void restart_cli()
{

  EmbeddedCliConfig *config = embeddedCliDefaultConfig();
  config->cliBuffer = cliBuffer;
  config->cliBufferSize = CLI_BUFFER_SIZE;
  config->rxBufferSize = CLI_RX_BUFFER_SIZE;
  config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
  config->historyBufferSize = CLI_HISTORY_SIZE;
  config->maxBindingCount = CLI_BINDING_COUNT;
  sprintf(prompt, "%s>", current->name);
  config->invitation = prompt;

  cli = embeddedCliNew(config);

  if (cli == NULL) {
    Serial.println(F("Cli was not created. Check sizes!"));
      
    uint16_t bindingCount = (uint16_t) (config->maxBindingCount + cliInternalBindingCount);
    sprintf(print_buffer, "Bindingcount=%d\r\n", bindingCount); Serial.print(print_buffer);

    size_t totalSize = embeddedCliRequiredSize(config);
    sprintf(print_buffer, "totalSize=%d\r\n", totalSize); Serial.print(print_buffer);


    return;
  }

  //Serial.println(F("Cli has started. Enter your commands."));
  /*
  embeddedCliAddBinding(cli, {
          "hello",
          "Print hello message",
          true,
          (void *) "World",
          onHello
  });
  */
  cli->onCommand = onCommand;
  cli->writeChar = writeChar;

}

bool started_cli_before = 0;
void init_cli()
{
  if ( started_cli_before ) 
  {
    // check state before just trying to terminate
    cliThread.terminate(); //  make sure this thread is dead 
  }

  EmbeddedCliConfig *config = embeddedCliDefaultConfig();
  config->cliBuffer = cliBuffer;
  config->cliBufferSize = CLI_BUFFER_SIZE;
  config->rxBufferSize = CLI_RX_BUFFER_SIZE;
  config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
  config->historyBufferSize = CLI_HISTORY_SIZE;
  config->maxBindingCount = CLI_BINDING_COUNT;

  cli = embeddedCliNew(config);

  if (cli == NULL) {
    Serial.println(F("Cli was not created. Check sizes!"));
      
    uint16_t bindingCount = (uint16_t) (config->maxBindingCount + cliInternalBindingCount);
    sprintf(print_buffer, "Bindingcount=%d\r\n", bindingCount); Serial.print(print_buffer);

    size_t totalSize = embeddedCliRequiredSize(config);
    sprintf(print_buffer, "totalSize=%d\r\n", totalSize); Serial.print(print_buffer);


    return;
  }

  Serial.println(F("Cli has started. Enter your commands."));
  /*
  embeddedCliAddBinding(cli, {
          "hello",
          "Print hello message",
          true,
          (void *) "World",
          onHello
  });
  */

  cli->onCommand = onCommand;
  cli->writeChar = writeChar;

  cliThread.start(cli_loop);
  started_cli_before = 1;
}


void onCommand(EmbeddedCli *embeddedCli, CliCommand *command) {
    Serial.println(F("Received command:"));
    Serial.println(command->name);
    embeddedCliTokenizeArgs(command->args);
    for (int i = 1; i <= embeddedCliGetTokenCount(command->args); ++i) {
        Serial.print(F("arg "));
        Serial.print((char) ('0' + i));
        Serial.print(F(": "));
        Serial.println(embeddedCliGetToken(command->args, i));
    }
}

void onHello(EmbeddedCli *cli, char *args, void *context) {
    Serial.print(F("Hello "));
    if (embeddedCliGetTokenCount(args) == 0)
        Serial.print((const char *) context);
    else
        Serial.print(embeddedCliGetToken(args, 1));
    Serial.print("\r\n");
}

void writeChar(EmbeddedCli *embeddedCli, char c) {
    Serial.write(c);
}




bool try_parse_hex_uint16t(const char* tok, uint16_t * val)
{
  if (tok[0] != '0' || (tok[1] != 'x' && tok[1] != 'X')) {
      return false;
  }

  // Ensure the string length is between 3 and 6 (0x + up to 4 hex digits)
  int length = strlen(tok);
  if (length < 3 || length > 6) {
      return false;
  }

  // Now check if the remaining characters are valid hex digits
  for (int i = 2; i < length; i++) {
      if (!isxdigit(tok[i])) {
          return false;
      }
  }

  // Optionally, parse the value to ensure it fits in 16 bits
  unsigned int value;
  if (sscanf(tok+2, "%x", &value) != 1 || value > 0xFFFF) {
      return false;
  }

  *val = (uint16_t) value;
  return true;
}

bool try_parse_hex_uint8t(const char* tok, uint8_t * val)
{
  if (tok[0] != '0' || (tok[1] != 'x' && tok[1] != 'X')) {
      return false;
  }

  // Ensure the string length is between 3 and 4 (0x + up to 2 hex digits)
  int length = strlen(tok);
  if (length < 3 || length > 4) {
      return false;
  }

  // Now check if the remaining characters are valid hex digits
  for (int i = 2; i < length; i++) {
      if (!isxdigit(tok[i])) {
          return false;
      }
  }

  // Optionally, parse the value to ensure it fits in 8 bits
  unsigned int value;
  if (sscanf(tok+2, "%x", &value) != 1 || value > 0xFF) {
      return false;
  }

  *val = (uint8_t) value;
  return true;
}

bool try_parse_hex_uint32t(const char* tok, uint32_t * val)
{
  if (tok[0] != '0' || (tok[1] != 'x' && tok[1] != 'X')) {
      return false;
  }

  // Ensure the string length is between 3 and 10 (0x + up to 8 hex digits)
  int length = strlen(tok);
  if (length < 3 || length > 10) {
      return false;
  }

  // Now check if the remaining characters are valid hex digits
  for (int i = 2; i < length; i++) {
      if (!isxdigit(tok[i])) {
          return false;
      }
  }

  // Optionally, parse the value to ensure it fits in 16 bits
  unsigned long value;
  if (sscanf(tok+2, "%lx", &value) != 1 || value > 0xFFFFFFFF) {
      return false;
  }

  *val = (uint32_t) value;
  return true;
}


























Node *change_directory(Node *current, const char *name) {
    if (strcmp(name, "..") == 0 && current->parent) {
        return current->parent;
    }
    
    for (int i = 0; i < current->num_children; i++) {
        if (strcmp(current->children[i]->name, name) == 0 && 
            current->children[i]->type == MY_DIRECTORY) {
            return current->children[i];
        }
    }
    Serial.println("Directory not found");
    return current;
}

void list_directory(Node *current) {
    for (int i = 0; i < current->num_children; i++) {
        sprintf(print_buffer, "%s\r\n", current->children[i]->name);
        Serial.print(print_buffer);
    }
}









/******************** Example of two file system entries *****************/
void i2c_read_operation(EmbeddedCli *cli, char *args, void *context);
static Node i2c_read_node = { .name = "read", 
  .type = MY_FILE, 
  .cliBinding = {"read",
    "I2C Read",
    true,
    nullptr,
    i2c_read_operation} 
};
void i2c_write_operation(EmbeddedCli *cli, char *args, void *context);
static Node i2c_write_node = { .name = "write", 
  .type = MY_FILE, 
  .cliBinding = {"write",
    "I2C write",
    true,
    nullptr,
    i2c_write_operation} 
};
// Define all files for the I2C directory

void i2c_dir_operation(EmbeddedCli *cli, char *args, void *context);
static Node * i2c_files[] = { &i2c_read_node, &i2c_write_node };
// Define the I2C directory
static Node i2c_dir = {
    .name = "i2c",
    .type = MY_DIRECTORY,
    .cliBinding = {"i2c",
          "I2C mode",
          true,
          nullptr,
          i2c_dir_operation},
    .parent = 0,
    .children = i2c_files,
    .num_children = sizeof(i2c_files) / sizeof(i2c_files[0])
};
void i2c_read_operation(EmbeddedCli *cli, char *args, void *context) {
    Serial.println("I2C read operation");
}
void i2c_write_operation(EmbeddedCli *cli, char *args, void *context) {
    Serial.println("I2C write operation");
}
void i2c_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  //Serial.println("Want to go into i2c mode!");
  change_to_node(&i2c_dir);
}

// Initialize function to set the parent pointers if needed
void i2c_init() {
  for (int i = 0; i < i2c_dir.num_children; i++) {
    i2c_files[i]->parent = &i2c_dir;
  }
  add_root_filesystem(&i2c_dir);
}





void spi_transfer_operation(EmbeddedCli *cli, char *args, void *context);
static Node spi_transfer = { .name = "transfer", 
  .type = MY_FILE, 
  .cliBinding = {"transfer",
    "SPI transfer",
    true,
    nullptr,
    spi_transfer_operation}
};

// Define all files for the SPI directory

void spi_dir_operation(EmbeddedCli *cli, char *args, void *context);
static Node * spi_files[] = { &spi_transfer };
// Define the SPI directory
static Node spi_dir = {
    .name = "spi",
    .type = MY_DIRECTORY,
    .cliBinding = {"spi",
          "SPI mode",
          true,
          nullptr,
          spi_dir_operation},
    .parent = 0,
    .children = spi_files,
    .num_children = sizeof(spi_files) / sizeof(spi_files[0])
};

void spi_transfer_operation(EmbeddedCli *cli, char *args, void *context) {
    Serial.println("SPI transfer operation");
}

void spi_dir_operation(EmbeddedCli *cli, char *args, void *context)
{
  //Serial.println("Want to change into SPI mode!");
  change_to_node(&spi_dir);
}

void spi_init() {
  for (int i = 0; i < spi_dir.num_children; i++) {
    spi_files[i]->parent = &spi_dir;
  }
  add_root_filesystem(&spi_dir);
}






// 30 max nodes below root, just hard coding for now
static Node * root_children[30];
static Node root = {
    .name = "/",
    .type = MY_DIRECTORY,
    .cliBinding = {"",
      "",
      true,
      nullptr,
      nullptr},
    .parent = 0,
    .children = root_children,
    .num_children = 0
};

void change_to_node(Node * n)
{
  if ( n != 0 && n != current ) {
    want_new_dir = n;
  }
}
void add_root_filesystem(Node * child)
{
  root_children[root.num_children] = child;
  root.num_children++;
  child->parent = &root;
}

void initialize_filesystem(void) {
    i2c_init();
    spi_init();
}

void onMenuCLI_cd(EmbeddedCli *cli, char *args, void *context)
{
  //Serial.println("onMenuCLI_cd!");
  Node * new_dir = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("onMenuCLI_cd no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) < 1 ) {
    Serial.println("onMenuCLI_cd needs 1 argument!");
    return;
  } 


  new_dir = change_directory(current, embeddedCliGetToken(args, 1) );
  if ( new_dir == current ) {
    // failed to find 
    return;
  }
  // found new directory
  // change to it , do this with a pointer so cli_loop can handle it
  // don't necessarily want to kill cli_loop thread, just want it to do the graceful swap
  want_new_dir = new_dir; 


}


void onMenuCLI_ls(EmbeddedCli *cli, char *args, void *context)
{
  //Serial.println("onMenuCLI_ls!");
  list_directory(current);
}

void addMenuCLI_standard_commands()
{
  embeddedCliAddBinding(cli, {
    "cd",
    "Change directory",
    true,
    nullptr,
    onMenuCLI_cd
  });

  embeddedCliAddBinding(cli, {
    "ls",
    "List current directory",
    true,
    nullptr,
    onMenuCLI_ls
  });
}

void addMenuCLI_current_directory() {
  for (int i = 0; i < current->num_children; i++) {
    embeddedCliAddBinding(cli, current->children[i]->cliBinding);
  }
}

void init_menu_cli()
{
  current = &root;

  init_cli();
  initialize_filesystem();
  addMenuCLI_standard_commands();
  //addMenuCLI_current_directory();
}





