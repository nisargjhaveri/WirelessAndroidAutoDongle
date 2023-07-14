mount -t debugfs none_debugs /sys/kernel/debug
echo 8 > /proc/sys/kernel/printk
echo '*:mod:usb_f_accessory' > /sys/kernel/debug/tracing/set_ftrace_filter
echo '*:mod:libcomposite' >> /sys/kernel/debug/tracing/set_ftrace_filter
echo function_graph > /sys/kernel/debug/tracing/current_tracer
tail -f /sys/kernel/debug/tracing/trace
