project(
    'cyberether-mohsen-plugin',
    ['cpp'],
    version: '0.1.0',
    meson_version: '>=1.11.0',
    default_options: [
        'cpp_std=c++20',
        'warning_level=3',
    ]
)

cyberether_dep = dependency(
    'cyberether',
    required: false,
    fallback: ['cyberether', 'libjetstream_dep'],
)

if not cyberether_dep.found()
    error('CyberEther dependency not found.')
endif

# Optional: the USRP probe block needs the SoapySDR headers and library. When
# they are missing the plugin builds without that block.
soapysdr_dep = dependency('SoapySDR', required: false)
plugin_soapysdr_enabled = soapysdr_dep.found()
plugin_soapysdr_deps = plugin_soapysdr_enabled ? [soapysdr_dep] : []

plugin_inc = include_directories('include', 'src')
plugin_sources = []

subdir('src')

shared_module(
    'cyberether_mohsen_plugin',
    plugin_sources,
    include_directories: plugin_inc,
    dependencies: [cyberether_dep] + plugin_soapysdr_deps,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: true,
    install: false,
)
