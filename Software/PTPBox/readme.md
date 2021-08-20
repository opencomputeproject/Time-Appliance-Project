## PTPBox

PTPBox is a bash script based framework which uses the Linux Network Namespace allowing it to create a PTP network on a single machine with multiple NICs. Network Namespace allows the machines to be isolated run run sricpts separately (similar to a container) and interract with other NICs via the physical connection. There are different nodes which can be confidugred (IP addresses and ports). Using a simple run.sh command you will have all individual threads run on sepearate Tmux panes and windows. It should be nted the LinuxPTP and Tmux are requirements for PTPBox to run properly. 

A block digram of a machine with 5 nodes looks as follows

<img width="520" alt="Screen Shot 2021-08-20 at 1 17 52 PM" src="https://user-images.githubusercontent.com/1751211/130289149-104930de-e346-4359-9b0a-2497c579d611.png">

Here is a picture of the Hardware implementation with 7 Nvidia CX6 NICs on a single machine.

![IMG_2883](https://user-images.githubusercontent.com/1751211/130288428-4c6ea350-f049-4743-a0a0-b58b781cf26e.jpg)

For a budget solution, a machine with multiple built in NICs can be utilized. Bellow you can see and implementation based on the [Protectli](https://protectli.com/) [Valut-6](https://protectli.com/vault-6-port/) firewall with six built-in NICs. The motherboard hosts six i210 NICs and an Intel i5 processor. Running four network namespaces with servos and master clock threads to simulate a two PTP enabled hops from GM to OC.
<img width="508" alt="Screen Shot 2021-08-20 at 1 19 53 PM" src="https://user-images.githubusercontent.com/1751211/130289603-5e0318b8-5fe6-41e6-afda-daed9a0e41e5.png">
<img width="380" alt="Screen Shot 2021-08-20 at 1 19 38 PM" src="https://user-images.githubusercontent.com/1751211/130289609-81e360cc-ace8-42a0-af31-23500b22dbc6.png">


# License
Contributions to this Specification are made under the terms and conditions set forth in Open Web Foundation Contributor License Agreement (“OWF CLA 1.0”) (“Contribution License”) by: 
 
 Facebook

You can review the signed copies of the applicable Contributor License(s) for this Specification on the OCP website at http://www.opencompute.org/products/specsanddesign 
Usage of this Specification is governed by the terms and conditions set forth in Open Web Foundation Final Specification Agreement (“OWFa 1.0”) (“Specification License”).   
 
You can review the applicable Specification License(s) executed by the above referenced contributors to this Specification on the OCP website at http://www.opencompute.org/participate/legal-documents/
 Notes: 
 
1)     The following clarifications, which distinguish technology licensed in the Contribution License and/or Specification License from those technologies merely referenced (but not licensed), were accepted by the Incubation Committee of the OCP:  
 
None

 
NOTWITHSTANDING THE FOREGOING LICENSES, THIS SPECIFICATION IS PROVIDED BY OCP "AS IS" AND OCP EXPRESSLY DISCLAIMS ANY WARRANTIES (EXPRESS, IMPLIED, OR OTHERWISE), INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, OR TITLE, RELATED TO THE SPECIFICATION. NOTICE IS HEREBY GIVEN, THAT OTHER RIGHTS NOT GRANTED AS SET FORTH ABOVE, INCLUDING WITHOUT LIMITATION, RIGHTS OF THIRD PARTIES WHO DID NOT EXECUTE THE ABOVE LICENSES, MAY BE IMPLICATED BY THE IMPLEMENTATION OF OR COMPLIANCE WITH THIS SPECIFICATION. OCP IS NOT RESPONSIBLE FOR IDENTIFYING RIGHTS FOR WHICH A LICENSE MAY BE REQUIRED IN ORDER TO IMPLEMENT THIS SPECIFICATION.  THE ENTIRE RISK AS TO IMPLEMENTING OR OTHERWISE USING THE SPECIFICATION IS ASSUMED BY YOU. IN NO EVENT WILL OCP BE LIABLE TO YOU FOR ANY MONETARY DAMAGES WITH RESPECT TO ANY CLAIMS RELATED TO, OR ARISING OUT OF YOUR USE OF THIS SPECIFICATION, INCLUDING BUT NOT LIMITED TO ANY LIABILITY FOR LOST PROFITS OR ANY CONSEQUENTIAL, INCIDENTAL, INDIRECT, SPECIAL OR PUNITIVE DAMAGES OF ANY CHARACTER FROM ANY CAUSES OF ACTION OF ANY KIND WITH RESPECT TO THIS SPECIFICATION, WHETHER BASED ON BREACH OF CONTRACT, TORT (INCLUDING NEGLIGENCE), OR OTHERWISE, AND EVEN IF OCP HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
