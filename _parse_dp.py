import xlrd

wb = xlrd.open_workbook(r'C:\Users\huche\Desktop\XiaoMi_F103\Core\tuya_protocol\数据点文档_海盈智慧屏_202607201440.xls')
sh = wb.sheet_by_name('数据点')

print(f'=== 海盈智慧屏 DP数据点完整列表 ===')
print(f'产品PID: y3gnk8n7')
print(f'数据行数: {sh.nrows - 6}')
print()
print(f'{"DP ID":<6} {"名称":<14} {"标识名":<24} {"传输方式":<10} {"类型":<8} {"数据属性/备注"}')
print('-' * 130)

for r in range(6, sh.nrows):
    dp_id_val = sh.cell_value(r, 0)
    name = str(sh.cell_value(r, 1))
    code = str(sh.cell_value(r, 2))
    trans_type = str(sh.cell_value(r, 3))
    dp_type = str(sh.cell_value(r, 4))
    attr = str(sh.cell_value(r, 5))
    remark = str(sh.cell_value(r, 6))

    # Handle non-numeric dp_id
    try:
        dp_id_str = str(int(dp_id_val))
    except (ValueError, TypeError):
        dp_id_str = str(dp_id_val)

    if dp_id_str and dp_id_str not in ('', '名词解释', 'None'):
        print(f'{dp_id_str:<6} {name:<14} {code:<24} {trans_type:<10} {dp_type:<8} {attr[:50] if attr else ""}')
        if remark and remark != '':
            print(f'      备注: {remark[:120]}')
        print()

print('--- 提取完成 ---')
