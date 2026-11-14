# AI Image Bridge 0.4.3: OpenAI API setup / OpenAI API準備マニュアル

Checked 2026-11-14; account screens may change. No key or payment action is performed by this manual. / 2026年11月14日確認。アカウント画面は変わる場合があります。この説明自体がキー作成や決済を実行することはありません。

## Prepare your account / アカウントの準備

1. Sign in to [OpenAI Platform](https://platform.openai.com/). / OpenAI Platformへログインします。
2. Check the selected organization and project. Use the same project for the key, billing checks, and model access. / 選択中の組織とプロジェクトを確認し、キー、課金、モデル利用権限を同じプロジェクトで確認します。
3. Open [Billing](https://platform.openai.com/settings/organization/billing/overview), follow the account's payment instructions, and check available balance. API billing is separate from ChatGPT subscriptions. / Billingを開き、アカウントに表示される支払い手順に従って利用可能残高を確認します。API課金はChatGPT月額契約と別です。
4. Review the [current API pricing](https://developers.openai.com/api/docs/pricing). Image model, image size, and requested quality affect cost; this app does not display a guaranteed price before generation. / 現行のAPI料金を確認します。画像モデル、サイズ、画質で料金が変わり、このアプリは生成前の確定金額を表示しません。

## Create the key / キーの作成

Open the [API keys page](https://platform.openai.com/api-keys), create a secret key for the intended project, and copy it into the plugin's masked field. Do not put it in a prompt, screenshot, source file, or shared document. / API keysページで目的のプロジェクトのシークレットキーを作成し、プラグインの伏字入力欄へ貼り付けます。指示文、スクリーンショット、ソースコード、共有資料へ貼り付けないでください。

Use key permissions that permit the app's model-list retrieval and image editing requests. A successful account login alone does not prove that the selected image model is enabled. / モデル一覧取得と画像編集リクエストを許可したキーを使います。アカウントへログインできても、選択した画像モデルを使えるとは限りません。

Source: [Official quickstart](https://developers.openai.com/api/docs/quickstart). / 出典：OpenAI公式クイックスタート。

## Connect in AI Image Bridge / アプリで接続

1. Select OpenAI, paste the key, and press the model-retrieval button. / OpenAIを選び、キーを貼り付けてモデル取得ボタンを押します。
2. Confirm the recognized state and model dropdown. The button becomes disabled until the key changes. / 認識済み表示とモデル一覧を確認します。キーを変更するまで送信ボタンは無効になります。
3. Use a saved, non-sensitive test layer. Choose Add, write a short instruction, and generate once. Model retrieval is connection checking, not an image-generation test. / 保存済みの機密性のないテストレイヤーで、追加を選び、短い指示を書いて1回生成します。モデル取得だけでは画像生成の動作確認は完了しません。
4. Inspect the result before applying. The app requests its configured high-quality settings; API fees apply even if you close without applying. / 結果を見てから適用します。アプリは設定された高画質条件を要求し、適用せず閉じてもAPI料金は発生します。

Keys remain only in the running CLIP STUDIO PAINT process. Reenter after restarting. Clearing the OpenAI key does not clear Gemini's key. / キーは起動中のCLIP STUDIO PAINTプロセス内だけで保持します。再起動後は再入力します。OpenAIのキーを消してもGeminiのキーは消えません。

## Troubleshooting / 接続できないとき

| Symptom / 症状 | Check / 確認すること |
| --- | --- |
| Authentication failure / 認証失敗 | Recopy the current key without spaces; check whether it was deleted or revoked. / 余分な空白を入れず現行キーを貼り直し、削除や失効を確認。 |
| Permission or model failure / 権限・モデルエラー | Check project permissions and availability of the selected image model. / プロジェクトの権限と対象画像モデルの利用可否を確認。 |
| Rate or quota failure / レート・利用枠エラー | Read the message: request frequency and exhausted billing quota require different fixes. Check usage and billing before retrying. / 頻度制限と残高・利用枠不足をメッセージで区別し、使用量と課金状態を確認。 |
| Network or server failure / 通信・サーバーエラー | Check HTTPS connectivity, status, and diagnostics. Wait before a deliberate retry. / HTTPS接続、稼働状況、診断記録を確認し、再試行は間隔を空ける。 |
| Safety refusal / 安全性による拒否 | Review the request against provider rules; the app does not bypass refusal. / 提供元のルールに照らして指示を確認。アプリは拒否を迂回しません。 |

Source: [Official error guide](https://developers.openai.com/api/docs/guides/error-codes). / 出典：公式エラーガイド。

If a key is exposed, revoke it in the dashboard and create a replacement. Share the run ID and sanitized error details when requesting help, never the key. / 漏えいしたキーは管理画面で失効させて作り直します。問い合わせには実行IDと秘密を除いたエラー内容を使い、キーは送らないでください。
