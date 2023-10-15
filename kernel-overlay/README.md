# What kernel?
This project runs on the Zephyr kernel

# Why do we need on overlays?
The standard nRF SDK contains everything we need to interface with the nRF9160, which is the SiP we're using.
The problem is that this SDK acts as a sort of "shim" between us and the Zephyr kernel, and doesn't integrate well with Zephyr's existing networking and location/GNSS stack. This causes a few problems since:

1. We want everything to be modular (ie: we don't want a bunch of if statements to see if we're running on Nordic hardware or not)
2. We want to use standard TLS drivers (like WolfSSL), and this is much easier/safer to integrate into the default networking stack.
3. All of the rest of Nordic's funtionality, like I2C and other sensors, are already upstreamed.

In addition, Zephyr still doesn't have good a good Location driver, we're waiting for this [PR](https://github.com/zephyrproject-rtos/zephyr/pull/61073) to merge, and will include it's functionality in this overlay in the meantime.

If this changes in the future, and Nordic decides to upstream their code and remove the shim, we won't need this overlay anymore.