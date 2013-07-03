2013/07/04 平岡 拓也

sfen 形式を Bonanza で学習する際の records.csa 形式に変換します。

環境は Linux, g++4.5 以上を想定しています。
それ以外の環境ではコンパイルエラーが出る箇所を適宜改変し使用して下さい。

ビルド、使用方法は以下のコマンドを実行して下さい。

$ cd src
$ make
$ ./sfentobonanza < ../sample.sfen > records.csa

sample.sfen を参考に棋譜を追記して使用して下さい。
ただし、行頭は必ず startpos にして下さい。
また、ファイル末尾は改行にしておかないと処理が終わらない可能性があります。

ソースコードの著作権は平岡拓也が保有します。
ライセンス形式は GPLv3 とします。

mail: hiraoka64@gmail.com
web : http://d.hatena.ne.jp/hiraoka64/
twitter: @HiraokaTakuya

---------------------------------------------------------
Bonanza のソースコードは以下の web site からダウンロード出来ます。
http://www.geocities.jp/bonanza_shogi/
