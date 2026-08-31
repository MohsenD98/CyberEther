project('soapysdr', 'cpp', version: '0.8.1', default_options: ['cpp_std=c++11', 'optimization=2'])

private_cpp_args = ['-DSOAPY_SDR_DLL_EXPORTS']
interface_cpp_args = []

if host_machine.system() == 'windows'
    module_ext = '.dll'
elif host_machine.system() == 'darwin'
    module_ext = '.dylib'
else
    module_ext = '.so'
endif

conf = configuration_data()
conf.set('SOAPY_SDR_ROOT_ENV', 'SOAPY_SDR_ROOT')
conf.set('CMAKE_INSTALL_LIBDIR', 'lib')
conf.set('MODULE_EXT', module_ext)
conf.set('SOAPY_SDR_ROOT', '')
conf.set('SOAPY_SDR_VERSION', meson.project_version())

modules_cpp = configure_file(
    input: 'lib/Modules.in.cpp',
    output: 'Modules.cpp',
    configuration: conf,
)

version_cpp = configure_file(
    input: 'lib/Version.in.cpp',
    output: 'Version.cpp',
    configuration: conf,
)

soapysdr_lib = shared_library(
  'SoapySDR',
  'lib/ConverterRegistry.cpp',
  'lib/ConvertersC.cpp',
  'lib/DefaultConverters.cpp',
  'lib/Device.cpp',
  'lib/DeviceC.cpp',
  'lib/Errors.cpp',
  'lib/ErrorsC.cpp',
  'lib/Factory.cpp',
  'lib/FactoryC.cpp',
  'lib/Formats.cpp',
  'lib/FormatsC.cpp',
  'lib/Logger.cpp',
  'lib/LoggerC.cpp',
  modules_cpp,
  'lib/ModulesC.cpp',
  'lib/NullDevice.cpp',
  'lib/Registry.cpp',
  'lib/TimeC.cpp',
  'lib/Types.cpp',
  'lib/TypesC.cpp',
  version_cpp,
  'lib/VersionC.cpp',
  cpp_args: private_cpp_args,
  include_directories: 'include',
  install: true,
)

soapysdr_dep = declare_dependency(
  include_directories: 'include',
  compile_args: interface_cpp_args,
  link_with: soapysdr_lib,
)

install_subdir('include/SoapySDR', install_dir: get_option('includedir'))
