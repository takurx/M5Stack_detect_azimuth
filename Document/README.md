# output 1 year, sun azimuth and altitude

## 概要
`test_year_sun_az_al-1.py` は、指定した位置における1年間の太陽の方位角と高度角を計算し、CSVファイルとして出力するPythonスクリプトです。

## 使い方

### 1. 位置情報の入力

スクリプト内の以下の部分で、太陽角度を計算したい場所の緯度（latitude）と経度（longitude）を設定します。

```python
# >>> input lattitude and longitude
# 例: 大阪の場合
ido = 34.55898169033403      # 緯度
keido = 135.50673654060122   # 経度
koko = EarthLocation(lat=ido, lon=keido)
```

**設定方法:**
- `ido` に緯度を度数（decimal degrees）で入力
- `keido` に経度を度数（decimal degrees）で入力
- 北緯・東経は正の値、南緯・西経は負の値で入力します

**参考例:**
- つくば: 緯度 35.6842, 経度 139.5369
- 大阪: 緯度 34.5590, 経度 135.5067

### 2. 日時情報の入力

スクリプト内の以下の部分で、計算開始日を設定します。

```python
# >>> input start time and time difference from GMT
nenngappi = '2026-01-01'  # 計算開始日（YYYY-MM-DD形式）
toki = astropy.time.Time(nenngappi) - 9*u.hour
```

**設定方法:**
- `nenngappi` に計算を開始したい日付を `'YYYY-MM-DD'` 形式で入力
- スクリプトは指定した日から365日間（1年間）の太陽角度を計算します
- 日本標準時（JST、UTC+9）での計算のため、`- 9*u.hour` でGMTとの時差を調整しています

### 3. CSVファイルの出力

以下のコマンドでスクリプトを実行し、結果を `info_sun_angle.csv` として保存します。

```bash
python test_year_sun_az_al-1.py > info_sun_angle.csv
```

**出力されるCSVファイルの内容:**
- 1日あたり289行のデータ（5分間隔で24時間分）
- 365日分のデータが出力されます
- 各行には: インデックス、日時、高度角、方位角が含まれます

### 4. SDカードへの保存と装置での使用

**手順:**

1. **CSVファイルの生成**
   ```bash
   python test_year_sun_az_al-1.py > info_sun_angle.csv
   ```

2. **SDカードへのコピー**
   - 生成された `info_sun_angle.csv` ファイルをSDカードのルートディレクトリにコピーします
   - SDカードはFAT32形式でフォーマットされている必要があります

3. **M5Stack装置での使用**
   - `info_sun_angle.csv` が保存されたSDカードをM5Stack装置に挿入します
   - 装置の電源を入れると、自動的にSDカードからCSVファイルを読み込みます
   - 装置は現在時刻に基づいて、CSVファイルから適切な太陽角度データを参照し、太陽追尾を行います

**注意事項:**
- SDカードは装置の電源を切った状態で挿入してください
- CSVファイルのファイル名は必ず `info_sun_angle.csv` としてください
- 位置情報が実際の設置場所と一致していることを確認してください

## 実行例

```bash
# デフォルト設定で実行（大阪、2026年1月1日から）
python test_year_sun_az_al-1.py > info_sun_angle.csv

# 別の日付で実行する場合は、スクリプト内のnenngappiを編集してから実行
# 例: nenngappi = '2024-04-15' に変更後
python test_year_sun_az_al-1.py > info_sun_angle.csv
```
