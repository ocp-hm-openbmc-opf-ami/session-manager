project(
  'session-management',
  'cpp',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23'
  ],
  license: 'Apache-2.0',
  version: '1.0',
  meson_version: '>=1.1.1',
)

deps = [
  dependency('systemd'),
  dependency('sdbusplus'),
  dependency('phosphor-logging'),
]
systemd = dependency('systemd')
install_data(
    ['service_files/xyz.openbmc_project.SessionManager.service'],
    ['service_files/dropbear-session-manager.service'],
    install_dir: systemd.get_variable('systemdsystemunitdir')
  )

executable('dropbear-manager', 'src/dropbear_manager.cpp',
    dependencies : [dependency('sdbusplus')],
    install: true,
)

executable(
  'session-management',
  'src/main.cpp',
  'src/kvm-session-manager.cpp',
  'src/web-session-manager.cpp',
  'src/vmedia-session-manager.cpp',
  'src/ssh-session-manager.cpp',
  include_directories: ['include'],
  cpp_args: ['-DSESSION_MANAGER_TESTS'],
  dependencies: deps,
  install: true,
)
