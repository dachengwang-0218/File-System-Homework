#!/bin/bash

echo "🚀 準備執行 10 次自動化 I/O 效能測試..."

# 1. 建立標題列 (Header) 並覆寫/建立 result.csv (這裡用 >)
echo "Sequential Read 1,Sequential Read 2,Sequential Read 3,Sequential Write 1,Sequential Write 2,Sequential Write 3,Random Read 1,Random Read 2,Random Read 3,Random Buffered Write 1,Random Buffered Write 2,Random Buffered Write 3,Random Sync Write 1,Random Sync Write 2,Random Sync Write 3" > result.csv

# 2. 開始跑 10 次迴圈
for i in {1..100}
do
    echo "⏳ 正在執行第 $i 次測試..."

    # 清除快取並執行 hw1
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
    ./hw1 > out1.txt

    # 清除快取並執行 hw2
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
    ./hw2 > out2.txt

    # 清除快取並執行 hw3
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
    ./hw3 > out3.txt

    # 用 awk 工具精準抓取每行的「數字」
    hw1_data=($(grep "sec" out1.txt | awk '{print $(NF-1)}'))
    hw2_data=($(grep "sec" out2.txt | awk '{print $(NF-1)}'))
    hw3_data=($(grep "sec" out3.txt | awk '{print $(NF-1)}'))

    # 將這一次的數據附加寫入 CSV (注意這裡是用 >> 附加到下一行)
    echo "${hw1_data[0]},${hw2_data[0]},${hw3_data[0]},${hw1_data[1]},${hw2_data[1]},${hw3_data[1]},${hw1_data[2]},${hw2_data[2]},${hw3_data[2]},${hw1_data[3]},${hw2_data[3]},${hw3_data[3]},${hw1_data[4]},${hw2_data[4]},${hw3_data[4]}" >> result.csv

done

# 3. 測試結束，清理暫存檔
rm out1.txt out2.txt out3.txt

echo "✅ 100 次測試全部完成！數據已成功匯出至 result.csv"
