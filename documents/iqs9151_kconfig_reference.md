# IQS9151 Driver Kconfig Reference

この文書は `drivers/input/Kconfig` の内容を、ドライバ全体の設定一覧として整理したものです。

## 1. Driver Core

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151`|bool|`y`|IQS9151ドライバ有効化|
|`CONFIG_INPUT_IQS9151_LOG_LEVEL`|int|`INPUT_LOG_LEVEL` (LOG有効時), それ以外 `0`|ドライバログレベル|
|`CONFIG_INPUT_IQS9151_INIT_PRIORITY`|int|`80`|ドライバ初期化優先度|

## 2. Rotation

`CONFIG_INPUT_IQS9151_ROTATE` choice により以下のいずれか1つを選択します（既定: `ROTATE_0`）。

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_ROTATE_0`|bool|`y` (choice既定)|回転なし|
|`CONFIG_INPUT_IQS9151_ROTATE_90`|bool|`n`|90度回転|
|`CONFIG_INPUT_IQS9151_ROTATE_180`|bool|`n`|180度回転|
|`CONFIG_INPUT_IQS9151_ROTATE_270`|bool|`n`|270度回転|

## 3. IC Parameter Overrides

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_RESOLUTION_X`|int|`2457`|X解像度設定（有効域 `0..4095`）|
|`CONFIG_INPUT_IQS9151_RESOLUTION_Y`|int|`3072`|Y解像度設定（有効域 `0..4095`）|
|`CONFIG_INPUT_IQS9151_ATI_TARGETCOUNT`|int|`400`|Trackpad ATIターゲット|
|`CONFIG_INPUT_IQS9151_DYNAMIC_FILTER_BOTTOM_SPEED`|int|`30`|Dynamic Filter Bottom Speed（有効域 `0..2047`）|
|`CONFIG_INPUT_IQS9151_DYNAMIC_FILTER_TOP_SPEED`|int|`511`|Dynamic Filter Top Speed（有効域 `0..2047`）|
|`CONFIG_INPUT_IQS9151_DYNAMIC_FILTER_BOTTOM_BETA`|int|`20`|Dynamic Filter Bottom Beta|

## 4. Gesture Detection and Thresholds

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_1F_TAP_ENABLE`|bool|`y`|1F Tap 有効/無効|
|`CONFIG_INPUT_IQS9151_1F_TAP_MAX_MS`|int|`500`|1F Tap/2回目Tap 判定の最大時間|
|`CONFIG_INPUT_IQS9151_1F_TAP_MOVE`|int|`50`|1F Tap 移動しきい値|
|`CONFIG_INPUT_IQS9151_1F_PRESSHOLD_ENABLE`|bool|`y`|1F deferred-click/drag 有効/無効|
|`CONFIG_INPUT_IQS9151_1F_TAPDRAG_GAP_MAX_MS`|int|`160`|1F Tap後にBTN0を保持して2回目タッチを待つ最大時間|
|`CONFIG_INPUT_IQS9151_2F_TAP_ENABLE`|bool|`y`|2F Tap 有効/無効|
|`CONFIG_INPUT_IQS9151_2F_TAP_MAX_MS`|int|`500`|2F Tap 最大時間|
|`CONFIG_INPUT_IQS9151_2F_TAP_MOVE`|int|`50`|2F Tap 移動しきい値（重心/距離）|
|`CONFIG_INPUT_IQS9151_2F_PRESSHOLD_ENABLE`|bool|`y`|2F deferred-click/drag 有効/無効|
|`CONFIG_INPUT_IQS9151_2F_TAPDRAG_GAP_MAX_MS`|int|`200`|2F Tap後にBTN1を保持して2回目2Fタッチを待つ最大時間|
|`CONFIG_INPUT_IQS9151_SCROLL_X_ENABLE`|bool|`y`|2F 横スクロール有効/無効|
|`CONFIG_INPUT_IQS9151_SCROLL_Y_ENABLE`|bool|`y`|2F 縦スクロール有効/無効|
|`CONFIG_INPUT_IQS9151_2F_SCROLL_START_MOVE`|int|`50`|2F Scroll 開始しきい値|
|`CONFIG_INPUT_IQS9151_2F_PINCH_ENABLE`|bool|`y`|2F Pinch 有効/無効|
|`CONFIG_INPUT_IQS9151_2F_PINCH_START_DISTANCE`|int|`100`|2F Pinch 開始しきい値|
|`CONFIG_INPUT_IQS9151_2F_PINCH_WHEEL_GAIN_X10`|int|`40`|2F Pinch `REL_WHEEL` ゲイン（x10）|
|`CONFIG_INPUT_IQS9151_3F_TAP_ENABLE`|bool|`y`|3F Tap 有効/無効|
|`CONFIG_INPUT_IQS9151_3F_TAP_MAX_MS`|int|`400`|3F Tap 最大時間|
|`CONFIG_INPUT_IQS9151_3F_TAP_MOVE`|int|`35`|3F Tap 移動しきい値|
|`CONFIG_INPUT_IQS9151_3F_PRESSHOLD_ENABLE`|bool|`y`|3F deferred-click/drag 有効/無効|
|`CONFIG_INPUT_IQS9151_3F_TAPDRAG_GAP_MAX_MS`|int|`200`|3F Tap後にBTN2を保持して2回目3Fタッチを待つ最大時間|
|`CONFIG_INPUT_IQS9151_3F_SWIPE_THRESHOLD`|int|`200`|3F Swipe しきい値|

### 4.1 Long-press Hold（長押しホールド）

指を置いたまま静止させて N ms でボタンを押下する方式。TapDrag とは独立で、
併用可能。既定では無効。

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_LONG_PRESS_HOLD_ENABLE`|bool|`n`|長押しホールド 有効/無効（1F=BTN0 / 2F=BTN1 / 3F=BTN2）|
|`CONFIG_INPUT_IQS9151_LONG_PRESS_HOLD_MS`|int|`500`|長押しホールド成立時間の共通既定値（`100..2000`）|
|`CONFIG_INPUT_IQS9151_1F_LONG_PRESS_HOLD_MS`|int|共通値|1F 長押しホールド成立時間の個別上書き|
|`CONFIG_INPUT_IQS9151_2F_LONG_PRESS_HOLD_MS`|int|共通値|2F 長押しホールド成立時間の個別上書き|
|`CONFIG_INPUT_IQS9151_3F_LONG_PRESS_HOLD_MS`|int|共通値|3F 長押しホールド成立時間の個別上書き|
|`CONFIG_INPUT_IQS9151_LONG_PRESS_HOLD_MOVE`|int|`100`|長押し判定中に許容する累積移動量（`0..500`）|

設定例（1本指だけ 350ms、2F/3F は 500ms のまま）:

```
CONFIG_INPUT_IQS9151_LONG_PRESS_HOLD_ENABLE=y
CONFIG_INPUT_IQS9151_LONG_PRESS_HOLD_MS=500
CONFIG_INPUT_IQS9151_1F_LONG_PRESS_HOLD_MS=350
```

## 5. Inertia

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_ENABLE`|bool|`y`|1Fカーソル慣性 有効/無効|
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_DECAY`|int|`950`|1Fカーソル慣性 減衰率|
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_RECENT_WINDOW_MS`|int|`60`|1Fカーソル慣性の recent-window 判定時間|
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_STALE_GAP_MS`|int|`35`|最終1F移動から release までの最大許容時間|
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_MIN_SAMPLES`|int|`2`|1Fカーソル慣性に必要な直近移動サンプル数|
|`CONFIG_INPUT_IQS9151_CURSOR_INERTIA_MIN_AVG_SPEED`|int|`10`|1Fカーソル慣性に必要な平均速度|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_ENABLE`|bool|`y`|2Fスクロール慣性 有効/無効|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_DECAY`|int|`980`|2Fスクロール慣性 減衰率|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_RECENT_WINDOW_MS`|int|`60`|2Fスクロール慣性の recent-window 判定時間|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_STALE_GAP_MS`|int|`35`|最終2Fスクロールから release までの最大許容時間|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_MIN_SAMPLES`|int|`1`|2Fスクロール慣性に必要な直近スクロールサンプル数|
|`CONFIG_INPUT_IQS9151_SCROLL_INERTIA_MIN_AVG_SPEED`|int|`4`|2Fスクロール慣性に必要な平均速度|

## 6. Test

|Symbol|Type|Default|役割|
| - | - | - | - |
|`CONFIG_INPUT_IQS9151_TEST`|bool|`n`|ZTEST用の内部テストフック有効化（`depends on ZTEST`）|
