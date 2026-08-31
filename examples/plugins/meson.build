plugin_test_lst = []

pymod = import('python')
py = pymod.find_installation(required: true)
plugin_bundler = files('blueprint/tools/bundler.py')

# Hadi

plugin_sources = []
subdir('hadi/src')
hadi_module = shared_module(
    'cyberether_hadi_plugin',
    plugin_sources,
    include_directories: include_directories('hadi/include', 'hadi/src'),
    dependencies: libjetstream_dep,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: false,
    install: false,
)
custom_target(
    'cyberether_hadi_plugin_cep',
    input: [hadi_module],
    output: 'cyberether_hadi_plugin.cep',
    command: [
        py,
        plugin_bundler,
        '--output', '@OUTPUT@',
        '--name', 'cyberether-hadi-plugin',
        '--version', '0.1.0',
        '--minimum-jetstream-version', meson.project_version(),
        '--target', 'path=@INPUT0@,system=' + host_machine.system() + ',device=cpu,arch=' + host_machine.cpu_family(),
    ],
    build_by_default: true,
    install: false,
)
sum_lst += {'Plugin Hadi': true}

# Mohsen

plugin_sources = []
subdir('mohsen/src')
mohsen_module = shared_module(
    'cyberether_mohsen_plugin',
    plugin_sources,
    include_directories: include_directories('mohsen/include', 'mohsen/src'),
    dependencies: libjetstream_dep,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: false,
    install: false,
)
custom_target(
    'cyberether_mohsen_plugin_cep',
    input: [mohsen_module],
    output: 'cyberether_mohsen_plugin.cep',
    command: [
        py,
        plugin_bundler,
        '--output', '@OUTPUT@',
        '--name', 'cyberether-mohsen-plugin',
        '--version', '0.1.0',
        '--minimum-jetstream-version', meson.project_version(),
        '--target', 'path=@INPUT0@,system=' + host_machine.system() + ',device=cpu,arch=' + host_machine.cpu_family(),
    ],
    build_by_default: true,
    install: false,
)
sum_lst += {'Plugin Mohsen': true}

# Blueprint

plugin_sources = []
subdir('blueprint/src')
blueprint_module = shared_module(
    'cyberether_blueprint_plugin',
    plugin_sources,
    include_directories: include_directories('blueprint/include', 'blueprint/src'),
    dependencies: libjetstream_dep,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: false,
    install: false,
)
custom_target(
    'cyberether_blueprint_plugin_cep',
    input: [blueprint_module],
    output: 'cyberether_blueprint_plugin.cep',
    command: [
        py,
        plugin_bundler,
        '--output', '@OUTPUT@',
        '--name', 'cyberether-blueprint-plugin',
        '--version', '0.1.0',
        '--minimum-jetstream-version', meson.project_version(),
        '--target', 'path=@INPUT0@,system=' + host_machine.system() + ',device=cpu,arch=' + host_machine.cpu_family(),
    ],
    build_by_default: true,
    install: false,
)
sum_lst += {'Plugin Blueprint': true}
