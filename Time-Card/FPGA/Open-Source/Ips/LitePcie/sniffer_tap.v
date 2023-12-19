module sniffer_tap (
    (* mark_debug = "true" *)
    input wire         rst_n_in,
    (* mark_debug = "true" *)
    input wire         clk_in,
    (* mark_debug = "true" *)
    input wire [15:0]  rx_data_in,
    (* mark_debug = "true" *)
    input wire [1:0]   rx_ctl_in,

    output wire        rst_n_out,
    output wire        clk_out,
    output wire [15:0] rx_data_out,
    output wire [1:0]  rx_ctl_out
);

    assign rst_n_out   = rst_n_in;
    assign clk_out     = clk_in;
    assign rx_data_out = rx_data_in;
    assign rx_ctl_out  = rx_ctl_in;

endmodule