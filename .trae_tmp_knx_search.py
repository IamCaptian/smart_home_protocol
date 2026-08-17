import openpyxl, json
wb = openpyxl.load_workbook(r"c:\Users\huche\Desktop\XiaoMi_F103\面板通信协议V1.0.xlsx", data_only=True)
for ws in wb.worksheets:
    for row in ws.iter_rows():
        vals = []
        hit = False
        for c in row:
            v = c.value
            s = '' if v is None else str(v)
            vals.append(s)
            if ('按键' in s) or ('公共显示' in s) or ('fun1' in s.lower()):
                hit = True
        if hit:
            print('SHEET:', ws.title)
            print('ROW', row[0].row, json.dumps(vals, ensure_ascii=False))
