# AI Image Bridge 0.4.1: Plugin manual / プラグインマニュアル

## Before installing / 導入前

AI Image Bridge edits the current color raster layer in CLIP STUDIO PAINT using your own OpenAI or Gemini API account. API charges apply. No Python, ComfyUI, or local GPU model is required. / AI Image Bridgeは、ご自身のOpenAIまたはGemini APIでCLIP STUDIO PAINTの現在のカラーラスターレイヤーを編集します。API利用料金が必要です。Python、ComfyUI、ローカルGPUモデルは不要です。

The ordinary public package is the unofficial SDK edition. The official SDK edition is being prepared for CELSYS review and is not an approved store release. Install only one edition. / 通常の公開パッケージは非公式SDK版です。公式SDK版はCELSYSへの審査準備中で、承認済みストア配布版ではありません。導入するのは一方だけです。

## Install and update / 導入と更新

1. Save your work and exit CLIP STUDIO PAINT. / 作業を保存してCLIP STUDIO PAINTを終了します。
2. Extract the whole release ZIP and double-click `Install.cmd`. / 配布ZIP全体を展開し、`Install.cmd`をダブルクリックします。
3. The plugin and PDFs are installed under `%APPDATA%\CELSYSUserData\CELSYS\CLIPStudioModule\PlugIn\PAINT\AIImageBridge`. / このフォルダーへプラグインとPDFを配置します。
4. Restart the host and select `Filter → AI Image Bridge (Unofficial SDK) → AI image editor...` / ホストを起動し、`フィルター → AI Image Bridge (Unofficial SDK) → AI画像編集...`を開きます。

The installer retains an old CloudImage installation under `%LOCALAPPDATA%\AIImageBridge\backups`. Save and exit before every update. API keys are not transferred across a host restart. / 旧CloudImageは`%LOCALAPPDATA%\AIImageBridge\backups`へ退避します。更新時も保存して終了してください。ホストを再起動するとAPIキーは引き継ぎません。

## Connect and choose language / 接続と言語

Use the language dropdown to choose 日本語 or English. Select the provider, enter its key, and retrieve models. A recognized key disables the retrieval button; editing the key enables it again. / 言語欄で日本語またはEnglishを選びます。提供元を選択してキーを入れ、モデルを取得します。認識済みなら送信は無効になり、キーを書き換えると再取得できます。

Each provider's key is held only in host-process memory. Switching providers or reopening the plugin retains it; exiting the host discards it. Clear affects only the selected provider after confirmation. / キーは提供元ごとにホストのメモリだけで保持します。切替や開き直しでは保持し、ホスト終了で破棄します。消去は確認後、現在の提供元だけに適用します。

## Edit a layer / レイヤーを編集

1. Duplicate the layer to keep the original and clear selections in the host. / 元を残すためレイヤーを複製し、ホスト側の選択範囲を解除します。
2. Choose Add, Replace, or Remove. / 追加、置き換え、消去を選びます。
3. Click or draw a rough stroke in the source preview if a position hint helps. The next click or stroke replaces the old hint. / 必要なら元画像をクリックするか線でなぞります。次の操作で前の指定を置き換えます。
4. Write the instruction and generate. A thin line or one point is enough to identify a target; the model sees the whole layer. / 指示を書いて生成します。対象の指定は細い線や一点でも構いません。AIはレイヤー全体を見ます。
5. Inspect the candidate, then Apply or Save PNG. Host Ctrl+Z can undo an applied edit. / 候補を確認して適用、またはPNG保存します。適用後はホストのCtrl+Zで戻せます。

| Mode / 内容 | Example / 指示例 |
| --- | --- |
| Add / 追加 | Add a waving girl on the left; keep the man. / 左に手を振る女の子を追加して。男性は残して。 |
| Replace / 置き換え | Replace the girl with a sitting brown dog. / 女の子を座った茶色い犬に置き換えて。 |
| Remove / 消去 | Remove the man and his shadow; repair the wall and floor. / 男性と影を消して、壁と床を補完して。 |

The pink area is an approximate position hint. Natural poses can extend beyond it. The result may also change details elsewhere, so inspect the complete image. / ピンクの表示は位置の目安です。自然なポーズが範囲を越えても構いません。ほかの場所にも差が出る場合があるため、全体を見て判断してください。

## References and history / 見本と履歴

Attach one reference for clothing, appearance, props, or style. Check its thumbnail and filename, then clear it when finished. For example: Add a girl wearing the clothes in this reference on the left. / 服装、外見、小物、画風の見本を1枚添えられます。サムネイルと名前で確認し、使い終わったら解除します。例：この見本の服を着た女の子を左に追加して。

Choose the original layer or the shown candidate as the next input. Undo/Redo restores retained candidates locally without API charges, including the first step back to the original. Up to eight recent candidates within approximately 256 MB are retained. A new generation after Undo replaces the forward history. / 次の入力を元レイヤーまたは表示中の候補から選べます。取り消しとやり直しはAPIを呼ばず、最初の生成も元画像へ戻せます。最新8候補、合計約256MBを目安に保持し、戻した位置で新規生成すると、その先の履歴は置き換わります。

Close without applying leaves the canvas unchanged and discards ordinary candidate history. Save PNG first to retain it. Cancel stops local communication; a submitted server request may still finish and incur charges. / 適用せず閉じるとキャンバスを変更せず、通常の候補履歴は消えます。残す場合は先にPNG保存します。通信中止後も送信済みのサーバー処理が続き、課金される場合があります。

## Existing selections and layer limits / 選択範囲とレイヤー制約

The plugin edits one raster layer, without automatic merging or a new layer. It rejects a selected layer mask and respects transparency lock. / 対象はラスターレイヤー1枚です。自動合成や新規レイヤー作成は行いません。マスク選択中は拒否し、透明度ロックを尊重します。

If a host selection was already active, use the deferred-apply control, close, press Ctrl+D, and reopen the same unchanged layer. A matching retained candidate is restored without generation. The source pixels, dimensions, and coordinates must match. The candidate disappears when the host exits. / ホスト側に選択範囲があった場合だけ、選択解除後に適用する操作で候補を保持し、閉じてCtrl+Dで解除し、変更していない同じレイヤーで開き直します。元画像の画素、寸法、座標が一致すれば再生成せず復元します。ホスト終了で保持候補は消えます。

## Settings and privacy / 設定とデータ

OpenAI requests Sunburst/max/high when available; Gemini requests 4K on supported models. Only an explicit unsupported-setting HTTP 400 enables one compatibility fallback per quality or complexity setting. General failures do not lower quality and regenerate. / OpenAIは利用可能な場合Sunburst/max/high、Geminiは対応モデルで4Kを要求します。設定未対応を明示するHTTP 400だけ、qualityとcomplexityの各設定で1回まで互換対応します。一般エラーで画質を下げて再生成しません。

The whole layer, instruction, position guide, and optional reference are sent to the chosen provider. Diagnostics exclude keys, prompts, images, and response bodies. / レイヤー全体、指示、位置の目安、見本画像を選択した提供元へ送信します。診断記録にキー、指示文、画像、応答本文は含めません。

Input maximum: 33,554,432 pixels; 16,384 px per side. Reference and response maximum: 64 MB each. Request maximum: 128 MB. Provider limits also apply. / 入力は33,554,432画素、各辺16,384pxまで。見本と応答は各64MB、送信は128MBまで。提供元の制限も適用されます。

## Help and removal / マニュアルと削除

The top-right Manual button opens a three-item menu for this manual, OpenAI setup, and Gemini setup. Each installed PDF opens in the operating system's default PDF viewer. / 右上のマニュアルボタンから、本書、OpenAI API準備、Gemini API準備を選べます。インストール済みPDFをOS既定のPDFビューアーで開きます。

To uninstall, exit the host and run the bundled `Uninstall.cmd`. Backups and user files are retained. / 削除時はホストを終了し、同梱の`Uninstall.cmd`を実行します。バックアップと利用者のファイルは保持します。

## Rights / 権利

The author claims no rights in your generated images. The code license is separate from output rights. API terms, applicable law, and third-party rights remain relevant. / 作者は利用者の生成画像への権利を主張しません。コードのライセンスと生成物の権利は別です。API条件、法令、第三者の権利は引き続き適用されます。

Gemini API terms require users to be at least 18, describe professional or business developer purposes rather than consumer use, and restrict availability by region. EEA, Switzerland, and UK API clients require Paid Services. Confirm your intended use before connecting. / Gemini API規約は18歳以上を条件とし、一般消費者用途ではなく業務・事業目的の開発者向け利用を定め、地域も制限しています。EEA、スイス、英国向けAPIクライアントには有料サービス条件があります。接続前にご自身の用途が条件に合うか確認してください。 [Terms / 規約](https://ai.google.dev/gemini-api/terms)

## Future SDK selection / 将来のSDK選択

Only the unofficial SDK edition is offered in the current public package. An installer choice for the official edition is conditional on explicit CELSYS permission for GitHub distribution and equivalent host testing; it is not a current feature. There is no runtime SDK switch in the editing screen. / 現在の公開パッケージは非公式SDK版だけです。公式版を選べるインストーラーは、CELSYSによるGitHub配布の明示許可と同等のホスト実機検証を得た後の条件付き対応で、現行機能ではありません。編集画面に実行時SDK切替はありません。

## Emergency stop / 緊急停止

During model lookup or image generation the red, white-text Emergency stop button closes the active local request. Pressing it disables it immediately; idle/stopped requests cannot be stopped again. Inputs and prior candidates are kept, and a new explicit request can run normally. Server-side work and charges may continue. / モデル取得・画像生成中は、赤地・白文字の緊急停止で通信を止められます。押した直後と待機中は非活性です。入力と以前の候補は保持し、次の明示操作で再実行できます。提供元側の処理・課金の取り消しは保証しません。

After API connection, emergency stop is visible at the top right (hidden during initial API setup). It is dark red and disabled while idle or after a stop request, and enabled only during model discovery or generation. Window fitting scales the client area while retaining native title-bar dimensions. / 初期API設定中は緊急停止を隠し、接続後は右上に表示します。待機中・停止要求後は暗赤色で非活性、モデル取得・生成中だけ有効です。小さな画面ではタイトルバーの実寸を残して操作領域を縮小します。

Previews contain the entire image within a visible frame and a 6-pixel inner gutter. Clicking the gutter does not add a position hint. Preview display does not crop or change source/output pixels. / プレビューは画像全体を枠と6pxの内余白に収めます。余白をクリックしても位置指定は付きません。表示の変更で元画像・出力画像を切り取りません。
