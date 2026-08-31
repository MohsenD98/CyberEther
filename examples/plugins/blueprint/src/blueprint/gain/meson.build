blueprint_gain_sources = files([
    'block_impl.cc',
    'module_benchmarks.cc',
    'module_impl.cc',
])

# Defined by the standalone plugin project. When the plugin is built in-tree by
# CyberEther, the variable is absent and the CPU implementation is always used.
if get_variable('plugin_device_cpu_enabled', true)
    blueprint_gain_sources += files(['module_impl_native_cpu.cc'])
endif

plugin_sources += blueprint_gain_sources

plugin_test_lst += files([
    'block_tests.cc',
    'module_tests.cc',
])
