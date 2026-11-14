# AI Image Bridge 0.4.3: Gemini API setup / Gemini API準備マニュアル

Checked 2026-11-14. Google account availability, billing, and model access vary by account and region. / 2026年11月14日確認。アカウントや地域によって利用可否、課金、モデルへのアクセスが異なります。

## Project and key / プロジェクトとキー

1. Sign in to [Google AI Studio](https://aistudio.google.com/) and open [API keys](https://aistudio.google.com/apikey). / Google AI Studioへログインし、API keysを開きます。
2. Create or select the intended project and create its API key using the current screen instructions. / 目的のプロジェクトを作成または選択し、画面の案内でAPIキーを作成します。
3. Keep the key private. Current official guidance distinguishes newer authorization keys from standard keys; follow the current key page rather than reusing an unrestricted old key. / キーを秘密に保ちます。公式資料では新しい認可キーと標準キーを区別しています。無制限の古いキーを流用せず、現在の画面案内に従ってください。

Each key is associated with a Google Cloud project. Verify that you are configuring the project used by the plugin's key. / 各キーはGoogle Cloudプロジェクトに関連付けられます。プラグインへ入れるキーと同じプロジェクトを設定してください。

Source: [Official key setup](https://ai.google.dev/gemini-api/docs/api-key). / 出典：公式キー設定ガイド。

## Billing and image-model access / 課金と画像モデル

Open [Projects](https://aistudio.google.com/projects) and follow the billing setup for the intended project. Current official guidance describes linking an active billing account and prepaying credits for the paid tier. Check the actual minimum, currency, and terms on your account before paying. / Projectsを開き、対象プロジェクトの課金設定へ進みます。現行の公式資料は、有効な課金アカウントの関連付けと、前払いクレジットによる有料枠への移行を案内しています。決済前に、ご自身の画面で最低金額、通貨、条件を確認してください。

A free AI Studio account or a paid Gemini consumer subscription is not proof that the requested image API model is available. Check the selected model in the [current pricing table](https://ai.google.dev/gemini-api/docs/pricing) and the project's billing state. / 無料のAI Studioアカウントや一般向けGeminiの有料契約だけでは、目的の画像APIモデルを使える証拠になりません。現行料金表とプロジェクトの課金状態を確認してください。

Sources: [Billing](https://ai.google.dev/gemini-api/docs/billing), [Pricing](https://ai.google.dev/gemini-api/docs/pricing). / 出典：課金ガイドと料金表。

## Connect in AI Image Bridge / アプリで接続

1. Select Gemini and paste the key into the masked key field. / Geminiを選び、伏字のキー欄へ貼り付けます。
2. Retrieve models and confirm recognition. An empty supported-model list is not a successful connection for this app. / モデルを取得して認識を確認します。対応モデル一覧が空なら、このアプリで使える接続の確認は完了していません。
3. Select an available image model. The app requests 4K for supported models; older 2.5 image models use model-defined resolution. / 利用できる画像モデルを選びます。対応モデルでは4K、旧2.5画像モデルではモデル規定の解像度を使います。
4. Generate once on a saved test layer with a short instruction. Inspect the preview before applying. / 保存済みのテストレイヤーで短い指示を1回生成し、プレビューを見てから適用します。

Model retrieval and paid image generation are separate checks. Keys remain only in the host process; reenter after CLIP STUDIO PAINT restarts. / モデル取得と有料画像生成は別の確認です。キーはホストのメモリだけに保持し、CLIP STUDIO PAINT再起動後は再入力します。

## Troubleshooting / エラーの確認

| Symptom / 症状 | Action / 対応 |
| --- | --- |
| Key or permission rejection / キー・権限の拒否 | Check key validity, its project, and allowed API. / キーの有効性、所属プロジェクト、許可APIを確認。 |
| Billing or depleted credits / 課金・クレジット不足 | Inspect the intended project's billing and available credits. / 対象プロジェクトの課金と残クレジットを確認。 |
| Resource exhausted / 利用枠超過 | Check model-specific limits and usage; wait where appropriate. / モデルごとの上限と使用量を確認し、必要なら待つ。 |
| Unsupported model or setting / モデル・設定未対応 | Retrieve models again and inspect the selected image model. / モデルを再取得して、選択中の画像モデルを確認。 |
| Unavailable or timeout / 一時障害・時間切れ | Check service/network state before retrying. Avoid repeated clicks. / サービスと回線を確認してから再試行。連打しない。 |
| Blocked generation / 生成拒否 | Review the provider's rules and the request. / 提供元の規則と依頼内容を確認。 |

Official troubleshooting distinguishes client/configuration errors from temporary failures. This app does not repeatedly regenerate on authentication, quota, safety, or general failures. / 公式トラブルシューティングは入力・設定の問題と一時障害を区別しています。このアプリは認証、利用枠、安全性拒否、一般エラーで繰り返し再生成しません。

Source: [Official troubleshooting](https://ai.google.dev/gemini-api/docs/troubleshooting). / 出典：公式トラブルシューティング。

Do not include the key in support requests. If exposed, revoke it and replace it in the app. / 問い合わせにキーを含めないでください。漏えいした場合は失効させ、アプリのキーを入れ替えます。

Gemini API terms require users to be at least 18, describe professional or business developer purposes rather than consumer use, and restrict availability by region. EEA, Switzerland, and UK API clients require Paid Services. Confirm your intended use before connecting. / Gemini API規約は18歳以上を条件とし、一般消費者用途ではなく業務・事業目的の開発者向け利用を定め、地域も制限しています。EEA、スイス、英国向けAPIクライアントには有料サービス条件があります。接続前にご自身の用途が条件に合うか確認してください。 [Terms / 規約](https://ai.google.dev/gemini-api/terms)
