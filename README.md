# NullCPUPowerManagement
NullCPUPowerManagement.kext tweaked to build on newer setups

***

# Original Sources

[AppleLife Repo](https://github.com/AppleLife/NullCPUPowerManagement)

[Original Source](https://tgwbd.org/darwin/extensions.html)

***

# Credits

* David Elliott for creating it
* [al3xtjames](https://github.com/al3xtjames) for code updates

***

NullCPUPowerManagement
======================

Another problem for OS X running on non-Apple hardware (virtual or otherwise) is the AppleIntelCPUPowerManagement kernel extension which tends to either panic the machine or spew endless debug messages to the console regarding the HPET and its relationship to the CPU.

In a virtual environment there is really no need for the guest OS to change processor C states. The built-in code which simply issues a halt instruction is sufficient. Furthermore, the virtualizer may wish to implement only enough of the HPET to get past the xnu startup routines without actually making the HPET work.

Therefore I have written the NullCPUPowerManagement extension. What it does is play a couple of tricks with the IOKit service registration process to ensure it takes over the AppleIntelCPUPowerManagement match category on its IOResources provider nub. The trick is that any nub matching on the IOResources nub must have a nub name identical to the value of its IOMatchCategory property. The nub name is by default the value of its IOClass property which must be its C++ class name and ought not to be AppleIntelCPUPowerManagement. Why not? Because the kernel cannot load two C++ classes with the same name. When this occurs, one or the other will win. In the best case, our copy would win. But in the worst case the real AppleIntelCPUPowerManagement wins and thus loads and wreaks havoc which is exactly what we are trying to avoid.

Therefore we use our own C++ class name but then from the init method we retrieve the value of the IOMatchCategory key and call setName to set the nub name to match. Combined with a probe score of 100 we guarantee that our provider (IOResources) will choose us over Apple's kext. Ideally we would implement an active probe method that did some checking to see if the system is capable of using AppleIntelCPUPowerManagement. Unfortunately it's hard to know exactly what conditions must be met. Therefore, this version (r11) always succeeds its probe unless you pass -allowAppleCPUPM on the command-line in which case it will fail its probe which allows AppleIntelCPUPowerManagement to attach.

The option isn't guaranteed to stay around, it's more for debugging purposes so you can keep the NullCPUPowerManagement kext in your Extensions folder while you play around trying to make Apple's CPU PM work. In case you don't get it right you can simply remove the -allowAppleCPUPM from your boot flags and you'll still be able to boot your system.
