import os

# true 使用cmake编译 false 使用g++编译
run_with_cmake = True
# 仅对g++生效，若为cmake脚本与CMakeLists.txt要在同一目录
code_path = 'src/'
test_case_path = 'testcases/'
# true 记录分支数量 输出顺序：成功率\n预测成功分支数\n预测总分支数 false 仅输出成功率
full_print = False

run_list = ['array_test1', 'array_test2', 'basicopt1', 'bulgarian', 'expr', 'gcd', 'hanoi', 'lvalue2', 'magic', 'manyarguments', 'multiarray', 'naive', 'pi', 'qsort', 'queens', 'statement_test', 'superloop', 'tak']
ans_list = []
base_md = ''
'''
if run_with_cmake:
    os.system('cmake CMakeLists.txt')
    os.system('make')
else:
    os.system('g++ src/base_run.cpp -o code -std=c++11')
'''
for test_name in run_list:
    print("now run "+test_name)
    run_cmd = os.popen('./code <'+test_case_path+test_name+'.data')
    ans_list.append(run_cmd.read())
    print("finish\n")

print('rate list: ')
print(ans_list)
print('begin build out file')
os.system('touch success_rate.md')
with open('success_rate.md', 'w') as f:
    if full_print:
        true_ans = []
        for i in ans_list:
            true_ans.append(i.split('\n'))
        f.write('## 分支预测成功率\n')
        f.write('| 文件名 | 成功率 | 预测成功分支数 | 预测分支总数 |\n')
        f.write('| :----: | :----: | :------------: | :----------: |\n')
        for i in range(len(run_list)):
            f.write('|'+run_list[i]+'|'+true_ans[i][0]+'|'+true_ans[i][1]+'|'+true_ans[i][2]+'|\n')
        f.write('`powered by walotta`')
    else:
        f.write('`powered by walotta`')
        f.write('## 分支预测成功率\n')
        f.write('| 文件名 | 成功率 |\n')
        f.write('| :----: | :----: |\n')
        for i in range(len(run_list)):
            f.write('|'+run_list[i]+'|'+ans_list[i]+'|\n')
        f.write('`powered by walotta`')

os.system('rm code')
print('run finish')
