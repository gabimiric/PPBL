# StepperMotor (archived)

Not wired into the active build. Lives here so PlatformIO won't compile it
unless a source file under `src/` actually `#include`s `StepperMotor.h`. If
you want to bring it back, add a registration line in
`ModuleManager::createEnabledModules()` and `#include "StepperMotor.h"`
in the calling source.
