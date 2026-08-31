# راهنمای ساخت پلاگین برای CyberEther

این راهنما فرآیند کامل ساخت یک بلوک جدید را — از ساختار فایل‌ها تا ثبت در UI — توضیح می‌دهد.  
پلاگین نمونه در این راهنما: **MohsenBLK** (passthrough ساده).

---

## ۱. مفاهیم اولیه

در CyberEther هر **بلوک** (Block) از دو لایه تشکیل شده:

| لایه | کلاس پایه | وظیفه |
|------|-----------|--------|
| **Block** | `Block::Config` + `Block::Impl` | رابط کاربری، ورودی/خروجی، پارامترها |
| **Module** | `Module::Config` + `Module::Impl` | منطق محاسباتی و کرنل اجرایی |

یک **پلاگین** یک shared library (`.dll` / `.so`) است که یک یا چند بلوک را ثبت می‌کند.

---

## ۲. ساختار فایل‌ها

```
examples/plugins/my_plugin/
├── include/
│   └── my_plugin/
│       └── my_block/
│           ├── block.hh              ← تعریف Block::Config
│           └── module.hh             ← تعریف Module::Config
├── src/
│   ├── plugin.cc                     ← ABI پلاگین (نقطه ورود DLL)
│   ├── meson.build
│   └── my_plugin/
│       └── my_block/
│           ├── block_impl.cc         ← پیاده‌سازی بلوک
│           ├── module_impl.hh
│           ├── module_impl.cc        ← پیاده‌سازی پایه ماژول
│           ├── module_impl_native_cpu.cc  ← کرنل CPU
│           └── meson.build
├── meson.build
└── subprojects/
    └── cyberether.wrap               ← فقط برای standalone build
```

---

## ۳. فایل‌های هدر

### `include/my_plugin/my_block/block.hh`

```cpp
#ifndef MY_PLUGIN_MY_BLOCK_BLOCK_HH
#define MY_PLUGIN_MY_BLOCK_BLOCK_HH

#include <jetstream/block.hh>
#include <jetstream/types.hh>

namespace Jetstream::Blocks {

struct MyBlock : public Block::Config {
    // پارامترهای قابل تنظیم (اگر وجود دارند)
    F32 gain = 1.0f;

    JST_BLOCK_TYPE(my_block);           // شناسه یکتا (snake_case)
    JST_BLOCK_DOMAIN("MyPlugin");       // دسته‌بندی در UI
    JST_BLOCK_PARAMS(gain);             // پارامترهای سریالایز‌شونده
    JST_BLOCK_DESCRIPTION(
        "My Block",                     // عنوان
        "یک جمله خلاصه.",              // خلاصه
        "# My Block\nتوضیحات کامل..."  // توضیحات markdown
    );
};

}  // namespace Jetstream::Blocks

#endif
```

> اگر بلوک پارامتر ندارد، `JST_BLOCK_PARAMS()` را بدون آرگومان بگذارید.

### `include/my_plugin/my_block/module.hh`

```cpp
#ifndef MY_PLUGIN_MY_BLOCK_MODULE_HH
#define MY_PLUGIN_MY_BLOCK_MODULE_HH

#include <jetstream/module.hh>
#include <jetstream/types.hh>

namespace Jetstream::Modules {

struct MyBlock : public Module::Config {
    F32 gain = 1.0f;

    JST_MODULE_TYPE(my_block);
    JST_MODULE_PARAMS(gain);
};

}  // namespace Jetstream::Modules

#endif
```

---

## ۴. پیاده‌سازی بلوک

### `src/my_plugin/my_block/block_impl.cc`

```cpp
#include <my_plugin/my_block/block.hh>
#include <jetstream/detail/block_impl.hh>
#include <my_plugin/my_block/module.hh>

namespace Jetstream::Blocks {

struct MyBlockImpl : public Block::Impl, public DynamicConfig<Blocks::MyBlock> {
    Result configure() override;
    Result define() override;
    Result create() override;

  protected:
    std::shared_ptr<Modules::MyBlock> moduleConfig = std::make_shared<Modules::MyBlock>();
};

Result MyBlockImpl::configure() {
    // مقادیر config بلوک را به ماژول منتقل کن
    moduleConfig->gain = gain;
    return Result::SUCCESS;
}

Result MyBlockImpl::define() {
    // تعریف ورودی‌ها، خروجی‌ها و پارامترهای قابل تنظیم در UI
    JST_CHECK(defineInterfaceInput("signal", "Signal", "ورودی"));
    JST_CHECK(defineInterfaceOutput("signal", "Signal", "خروجی"));
    JST_CHECK(defineInterfaceConfig("gain", "Gain", "ضریب تقویت", "float::3"));
    return Result::SUCCESS;
}

Result MyBlockImpl::create() {
    // ایجاد ماژول و اتصال ورودی/خروجی
    JST_CHECK(moduleCreate("my_block", moduleConfig, {
        {"signal", inputs().at("signal")},
    }));
    JST_CHECK(moduleExposeOutput("signal", {"my_block", "signal"}));
    return Result::SUCCESS;
}

JST_REGISTER_BLOCK(MyBlockImpl);

}  // namespace Jetstream::Blocks
```

---

## ۵. پیاده‌سازی ماژول

### `src/my_plugin/my_block/module_impl.hh`

```cpp
#ifndef MY_PLUGIN_MY_BLOCK_MODULE_IMPL_HH
#define MY_PLUGIN_MY_BLOCK_MODULE_IMPL_HH

#include <my_plugin/my_block/module.hh>
#include <jetstream/detail/module_impl.hh>

namespace Jetstream::Modules {

struct MyBlockImpl : public Module::Impl, public DynamicConfig<MyBlock> {
  public:
    Result define() override;
    Result create() override;
    Result reconfigure() override;

  protected:
    Tensor input;
    Tensor output;
};

}  // namespace Jetstream::Modules

#endif
```

### `src/my_plugin/my_block/module_impl.cc`

```cpp
#include "module_impl.hh"

namespace Jetstream::Modules {

Result MyBlockImpl::define() {
    JST_CHECK(defineTaint(Module::Taint::DISCONTIGUOUS));
    JST_CHECK(defineInterfaceInput("signal"));
    JST_CHECK(defineInterfaceOutput("signal"));
    return Result::SUCCESS;
}

Result MyBlockImpl::create() {
    input = inputs().at("signal").tensor;

    JST_CHECK(output.create(input.device(), input.dtype(), input.shape()));
    JST_CHECK(output.propagateAttributes(input));

    outputs()["signal"].produced(name(), "signal", output);
    return Result::SUCCESS;
}

Result MyBlockImpl::reconfigure() {
    const auto& config = *candidate();

    // اگر gain تغییر کرده، همان مقدار را به‌روز کن؛ وگرنه بازسازی کامل
    if (config.gain != gain) {
        gain = config.gain;
        return Result::SUCCESS;
    }
    return Result::RECREATE;
}

}  // namespace Jetstream::Modules
```

### `src/my_plugin/my_block/module_impl_native_cpu.cc`

```cpp
#include <functional>
#include <jetstream/module_context.hh>
#include <jetstream/registry.hh>
#include <jetstream/runtime_context_native_cpu.hh>
#include <jetstream/scheduler_context.hh>
#include <jetstream/tools/automatic_iterator.hh>
#include "module_impl.hh"

namespace Jetstream::Modules {

struct MyBlockImplNativeCpu : public MyBlockImpl,
                              public NativeCpuRuntimeContext,
                              public Scheduler::Context {
  public:
    Result create() final;
    Result computeSubmit() override;

  private:
    Result kernelF32();
    std::function<Result()> kernel;
};

Result MyBlockImplNativeCpu::create() {
    JST_CHECK(MyBlockImpl::create());

    if (input.dtype() == DataType::F32 && output.dtype() == DataType::F32) {
        kernel = [this]() { return kernelF32(); };
        return Result::SUCCESS;
    }

    JST_ERROR("[MY_BLOCK_NATIVE_CPU] Unsupported data type '{}'.", input.dtype());
    return Result::ERROR;
}

Result MyBlockImplNativeCpu::computeSubmit() {
    return kernel();
}

Result MyBlockImplNativeCpu::kernelF32() {
    const F32 scale = gain;
    return AutomaticIterator<F32, F32>(
        [scale](const auto& in, auto& out) { out = in * scale; },
    input, output);
}

// ثبت ماژول برای CPU Native
JST_REGISTER_MODULE(MyBlockImplNativeCpu, DeviceType::CPU, RuntimeType::NATIVE, "generic");

}  // namespace Jetstream::Modules
```

---

## ۶. نقطه ورود پلاگین

### `src/plugin.cc`

```cpp
#include <jetstream/plugin.hh>

JST_REGISTER_PLUGIN();
```

این ماکرو `jetstream_plugin_abi` symbol را export می‌کند که CyberEther هنگام لود dll
آن را می‌یابد. رکورد ABI فقط magic، اندازه و نسخهٔ ABI را دارد و هیچ متادیتایی
مثل نام یا نسخهٔ پلاگین در آن نیست — آن‌ها در `manifest.yml` داخل بستهٔ `.cep`
قرار می‌گیرند.

---

## ۷. فایل‌های Build

### `meson.build` (ریشه پلاگین)

```meson
project('cyberether-my-plugin', ['cpp'],
    version: '0.1.0',
    meson_version: '>=1.11.0',
    default_options: ['cpp_std=c++20', 'warning_level=3']
)

cyberether_dep = dependency('cyberether',
    required: false,
    fallback: ['cyberether', 'libjetstream_dep'],
)

if not cyberether_dep.found()
    error('CyberEther dependency not found.')
endif

plugin_inc = include_directories('include', 'src')
plugin_sources = []

subdir('src')

shared_module('cyberether_my_plugin', plugin_sources,
    include_directories: plugin_inc,
    dependencies: cyberether_dep,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: true,
    install: false,
)
```

### `src/meson.build`

```meson
plugin_sources += files('plugin.cc')
subdir('my_plugin/my_block')
```

### `src/my_plugin/my_block/meson.build`

```meson
plugin_sources += files([
    'block_impl.cc',
    'module_impl.cc',
    'module_impl_native_cpu.cc',
])
```

---

## ۸. ادغام با بیلد اصلی

برای اینکه پلاگین با `meson compile -C build cyberether_my_plugin` بیلد شود، یک بلوک به `examples/plugins/meson.build` اضافه کنید:

```meson
plugin_sources = []

subdir('my_plugin/src')

shared_module('cyberether_my_plugin', plugin_sources,
    include_directories: include_directories('my_plugin/include', 'my_plugin/src'),
    dependencies: libjetstream_dep,
    gnu_symbol_visibility: 'hidden',
    name_prefix: '',
    build_by_default: true,
    install: false,
)

sum_lst += {'Plugin my_plugin': true}
```

---

## ۹. بیلد

### بار اول (فعال‌سازی examples)

```bat
meson setup --reconfigure build -Dexamples=true
```

### بیلد فقط پلاگین‌ها (سریع)

```bat
meson compile -C build cyberether_my_plugin
```

یا با اسکریپت آماده:

```bat
build_plugins.bat
```

خروجی: `build\examples\plugins\cyberether_my_plugin.dll`

---

## ۱۰. ثبت پلاگین در CyberEther

بیلد شدن dll کافی نیست — باید یک‌بار در CyberEther ثبت شود تا از آن به بعد خودکار لود شود.

```
CyberEther → Settings → Registry → Add Plugin
```

| مرحله | توضیح |
|-------|--------|
| ۱ | CyberEther را اجرا کنید |
| ۲ | از منوی **Settings** وارد **Registry** شوید |
| ۳ | روی **Add Plugin** کلیک کنید |
| ۴ | فایل dll را از مسیر `build\examples\plugins\` انتخاب کنید |
| ۵ | تیک **"I trust the source of this plugin"** را بزنید |
| ۶ | **Register Plugin** را کلیک کنید |
| ۷ | اپ را ریستارت کنید |

پس از ثبت، مسیر dll در `%APPDATA%\CyberEther\settings.yaml` ذخیره می‌شود و دفعات بعد خودکار لود خواهد شد.

بلوک در flowgraph editor زیر دامنه‌ای که در `JST_BLOCK_DOMAIN` تعریف کردید ظاهر می‌شود.

---

## ۱۱. خلاصه ماکروهای کلیدی

| ماکرو | محل استفاده | کاربرد |
|-------|------------|--------|
| `JST_BLOCK_TYPE(name)` | `block.hh` | شناسه یکتا بلوک (snake_case) |
| `JST_BLOCK_DOMAIN(name)` | `block.hh` | دسته در UI |
| `JST_BLOCK_PARAMS(...)` | `block.hh` | پارامترهای سریالایز‌شونده |
| `JST_BLOCK_DESCRIPTION(...)` | `block.hh` | عنوان، خلاصه، توضیحات |
| `JST_MODULE_TYPE(name)` | `module.hh` | شناسه ماژول |
| `JST_MODULE_PARAMS(...)` | `module.hh` | پارامترهای ماژول |
| `JST_REGISTER_BLOCK(T)` | `block_impl.cc` | ثبت بلوک در رجیستری |
| `JST_REGISTER_MODULE(T, device, runtime, tag)` | `module_impl_native_cpu.cc` | ثبت ماژول برای device مشخص |
| `JST_REGISTER_PLUGIN()` | `plugin.cc` | export symbol برای dll loader |
| `JST_REGISTER_BLOCK_NO_MODULES(T)` | `block_impl.cc` | ثبت بلوکی که ماژول ندارد |
| `JST_BLOCK_NODE_SIZE(SIZE)` | `block.hh` | اندازه پیش‌فرض نود (`XS`..`XL`) |

---

## ۱۲. مثال دوم: بلوک بدون ماژول — USRP Probe

بلوک `MohsenBLK` الگوی متداول است: بلوک + ماژول + کرنل. ولی همه بلوک‌ها ماژول
لازم ندارند. اگر بلوک هیچ تنسوری پردازش نمی‌کند و فقط اطلاعات نشان می‌دهد،
می‌توان کلاً ماژول را حذف کرد.

بلوک `mohsen_usrp_probe` (در `examples/plugins/mohsen/.../usrp_probe/`) دستگاه‌های
USRP متصل را پیدا می‌کند و گزارش را به صورت markdown داخل خود نود نمایش می‌دهد.
هیچ ورودی و خروجی سیگنالی ندارد.

### تفاوت‌ها با بلوک معمولی

| موضوع | بلوک معمولی | بلوک بدون ماژول |
|--------|-------------|-----------------|
| فایل‌ها | `block.hh` + `module.hh` + ۳ فایل پیاده‌سازی | فقط `block.hh` + `block_impl.cc` |
| ثبت | `JST_REGISTER_BLOCK(T)` | `JST_REGISTER_BLOCK_NO_MODULES(T)` |
| `create()` | `moduleCreate(...)` و `moduleExposeOutput(...)` | فقط `return Result::SUCCESS;` |
| محل منطق | `module_impl_native_cpu.cc` | `configure()` داخل خود بلوک |

`configure()` هر بار که بلوک ساخته می‌شود یا یکی از پارامترهایش تغییر می‌کند
اجرا می‌شود — به همین دلیل اسکن دستگاه‌ها آنجا انجام می‌شود و پارامتر `rescan`
صرفاً یک کلید است که با تغییرش `configure()` دوباره صدا زده شود.

### نمایش خروجی در نود

فیلد پیکربندی با فرمت `markdown` یک ویرایشگر/نمایشگر markdown داخل نود می‌سازد:

```cpp
JST_CHECK(defineInterfaceConfig("report",
                                "Detected Devices",
                                "Result of the last scan.",
                                "markdown"));
```

بلوک کافی است در `configure()` مقدار همان پارامتر را بنویسد:

```cpp
Result MohsenUsrpProbeImpl::configure() {
    report = scan();
    return Result::SUCCESS;
}
```

فرمت‌های دیگری که ویرایشگر پشتیبانی می‌کند: `text`، `multiline`، `bool`،
`int`، `uint`، `float`، `range`، `dropdown:a,b,c`، `filepicker:ext1,ext2`،
`filesave`، `vector`، `vector-inline`، `tensor-config`، `python`.

### وابستگی خارجی (SoapySDR)

این بلوک مستقیم با SoapySDR کار می‌کند، پس پلاگین باید به آن لینک شود. چون
وابستگی اختیاری است، در meson با یک متغیر نگهبان کنترل می‌شود:

```meson
# examples/plugins/meson.build
plugin_soapysdr_deps = []
plugin_soapysdr_enabled = false
if cfg_lst.get('JETSTREAM_LOADER_SOAPY_AVAILABLE', false)
    plugin_soapysdr_deps += soapysdr_dep
    plugin_soapysdr_enabled = true
endif
```

```meson
# .../usrp_probe/meson.build
if get_variable('plugin_soapysdr_enabled', false)
    plugin_sources += files(['block_impl.cc'])
endif
```

اگر SoapySDR در بیلد نباشد، پلاگین همچنان کامپایل می‌شود و فقط این بلوک را
ندارد.

> نکته دربارهٔ زمان اجرا: پلاگین به `SoapySDR.dll` لینک می‌شود، ولی لازم نیست
> این فایل کنار پلاگین کپی شود. چون `cyberether.exe` خودش همان DLL را از قبل در
> پروسه لود کرده، ویندوز هنگام لود پلاگین همان نسخه را resolve می‌کند و هر دو
> یک registry مشترک می‌بینند.
