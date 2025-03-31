# ローマ・インターステラー・パラドックス  
（Rome Interstellar Paradox）

## 概要（Overview）

本作は、時間を旅する異星のローマ兵を主人公にしたシングルプレイヤーの2Dアクションゲームです。  
プレイヤーは、過去・現在・未来のローマを巡りながら、敵と戦い、時空を超えて繰り返される謎の侵略の真相を追います。

*Rome Interstellar Paradox is a single-player 2D action game featuring a time-traveling alien Roman soldier. The Player battles enemies across the past, present, and future of Rome, uncovering the mystery behind repeated invasions spanning different timelines.*

---

## ウォークスルー動画（Walkthrough Videos）

- 日本語版プレイ動画：[YouTubeリンク](https://www.youtube.com/watch?v=XMydQDvIuoQ)
- English Gameplay Video: [YouTube Link](https://www.youtube.com/watch?v=46V2ffLu4Bc)

---

## ゲーム基本情報（Game Information）

- **ゲームタイトル（Game Title）**：
  ローマ・インターステラー・パラドックス（和訳：ローマの星間パラドックス）

- **ジャンル（Genre）**：
  2Dプラットフォーマー（会話・ストーリー重視）＋アクション（剣・射撃・防御）

- **エンジン（Engine）**：
  SFMLを用いた自作C++ゲームエンジン

- **ゲームの特徴（Features）**：
  - ブラックホールを通じて異なる時代・宇宙を移動するタイムトラベル型ステージ構成
  - プレイヤーの行動が未来に影響を与える“パラドックス”を取り入れたストーリー展開

- **テーマ（Theme）**：
  時空を超えて旅するエイリアン・ローマ兵の物語。ブラックホールを通じて異なる宇宙と時代を巡りながら、真実を追う冒険。

- **制作動機（Motivation）**：
  小学生の頃からゲームが大好きで、「ゲームはどう作られているのか」を学びたくなりました。Game Freakに本気で入りたいという思いから、C++やゲーム設計を独学し、本作を制作しました。

---

## ストーリー概要（Story Overview）

- プレイヤーは異なる進化を遂げたローマ帝国のエイリアン兵士。
- ある日、突如として未来のローマ兵たちによる侵略を受け、次元の混乱から逃れ、時空を越えて真実を追う旅が始まる。
- プレイヤーの行動が未来の構造を変化させ、物語にパラドックスが生まれる。
- 最後には、また別の宇宙で復讐を誓うローマ兵が旅を始め、歴史は無限に繰り返される。

---

## ストーリー着想・テーマ（Story Inspiration & Themes）

- 一般相対性理論を学び始め、ブラックホールや多元宇宙に興味を持つ。
- ブラックホールによって異なる次元（古代ローマ、エイリアンローマ、未来ローマ）をつなぐ発想。
- 未来ローマでは、ブラックホール物質を武器や防具に応用する設定。
- ローマの美学（緑・白・金）とネビュラ（暗紫系）の融合デザイン。
- 過去の出来事が未来に影響する“パラドックス”の概念を導入。
- 『トロン』のような近未来ビジュアルを参考に未来ローマをデザイン。

---

## ゲームメニュー構成（Game Menu Structure）

- **言語選択（Language Selection）**：
  起動時に英語または日本語を選択可能。

- **モード選択（Game Modes）**：
  - ストーリーモード
  - 単体レベルプレイ
  - レベルエディター

- **レベルエディター機能（Level Editor Features）**：
  - グリッドシステムでオリジナルステージ作成可能。
  - 作成したレベルはテキストファイル（.txt）で保存。
  - 各タイルの座標とサイズ（96ピクセル）で構成。

---

## レベル構成（Level Structure：全12ステージ）

### エイリアンローマ（Alien Rome） - 2ステージ
- ゲーム導入部。プレイヤーは逃げながら世界観を理解する。

### 古代ローマ（Ancient Rome） - 3ステージ＋ボス＋特殊ステージ
- 近接戦が中心。剣と盾を使った戦闘がメイン。
- ボス戦あり。
- 特殊ステージ（1ステージ）：最初のレベルの逆走バージョン（右→左）。

### 未来ローマ（Future Rome） - 3ステージ＋ボス＋特殊ステージ
- 古代ローマに近い構成だが、技術や演出が進化。
- プレイヤー能力や敵の戦術も変化。

- 背景画像はGPT-4o（生成AI）を活用して制作。
---

## Screenshots

### レベル1：エイリアン・ローマ (Level 1: Alien Rome)

![Alien Rome Level](https://github.com/ludovicocuoghi/roman_paradox/blob/main/screenshots/alien_rome_1.png?raw=true)

## 現在の課題（Current Issues）

プレイヤーや敵が積み重なったタイルと衝突した際、横方向に移動すると空中に浮いたままタイルに引っかかる場合があります。これは衝突処理を横方向・縦方向で同時に処理しているため起きる現象ですが、X軸の移動を完全に制限する修正を行うとジャンプや登り動作がぎこちなくなります。そのため、プレイヤー体験を損なわないよう、縦横の衝突判定を個別に処理するなどの、より自然でスムーズな解決方法を模索中です。

*When the player or enemies collide with stacked tiles and move horizontally, they can become stuck floating mid-air on tiles. This occurs due to simultaneously processing horizontal and vertical collisions. Completely restricting horizontal movement solves the issue but negatively affects jumping and climbing smoothness. Therefore, a more natural and smoother solution, such as separately handling vertical and horizontal collisions, is currently being explored.*

---

## 今後の展開（Future Plans）

本作は、C++およびゲーム開発スキルを向上させるために作成したプロジェクトです。今後は特に以下の点に注力して改善・拡張を行う予定です。

- レベルB・Cバグの特定と修正、全体のパフォーマンス最適化  
  *(Identifying and fixing level B and C bugs, optimizing overall game performance.)*
- 衝突判定システムの精度向上、特に複雑なシナリオでの挙動改善  
  *(Improving collision system accuracy, especially in complex scenarios.)*
- 会話シーンの表示位置や発生タイミングを最適化し、ストーリーの没入感向上  
  *(Optimizing dialogue placement and timing to enhance narrative immersion.)*
- 新たな時代や追加ステージの設計と実装によるプレイ体験の多様化  
  *(Designing and implementing additional eras and stages to enrich gameplay variety.)*
- 敵AIの行動パターンをさらに洗練させ、新規ボスキャラクターの導入  
  *(Refining enemy AI behaviors and introducing new boss characters.)*
- プレイヤー自身が自由にステージを設計・共有できるステージエディタの機能拡張  
  *(Expanding the stage editor functionality to allow players to freely design and share custom stages.)*
- プレイヤーの選択によってストーリーが分岐し、複数のエンディングが楽しめる仕組みの導入  
  *(Implementing branching storylines and multiple endings based on player decisions.)*

これらの取り組みを通じて、ゲーム開発に関する実践的な理解を深め、さらなるスキルアップを図っていきます。

*Through these initiatives, I'll further deepen my practical understanding of game development and continue advancing my skills.*

---

本作は C++ / SFML / ImGui を用いて、アートから実装まですべて個人で開発しました。  
ぜひプレイしてみてください！

*This game was individually developed from art to implementation using C++, SFML, and ImGui. Please give it a try!*
