###################  filter.ctl ##################################
#
#      ctlinnbbsd reload 
#
#      First Match Applied
#
# Available filters:
# 1. big2gb: BIG5 to GB(8bits) 
# 2. gb2big: GB(8bits) to BIG5
# 3. hz2gb:  HZ to GB
# 4. hz2big: HZ to BIG5
# 5. external commands in forms of /xx/xx/xx
#
#     Todo filters:
# 1. jis2big: JIS to BIG5 (not
# 2. big2jis: BIG to JIS
# 3. u8tob5: Unicode to Big5 
# 4. b5tou8: BIG5 to Unicode
# 5. pipe stream in yy2zz|zz2ww
##################################################################
#
# group wild-pattern,..:receiving filter:sending filter
##################################################################
############ for BBS in GB environment to carry tw.bbs.* 
############ alt.chinese.text and alt.chinese.text.big5
#tw.bbs.*,alt.chinese.text.big5,cna.*,csie.test:big2gb:gb2big 
#alt.chinese.text.big5:big2gb:gb2big
#alt.chinese.text:hz2gb:/usr/local/bin/gb2hz
##################################################################
######### for BBS in BIG5 to carry alt.chinese.text
#alt.chinese.text:hz2big:/usr/local/bin/b2g|/usr/local/bin/gb2hz
##################################################################
######### to receive fj.* in jis and convert into BIG5
#fj.*:/usr/local/bin/j2b:/usr/local/bin/b2j
#########
#chinese.txt.unicode:/usr/local/bin/u8tob5:/usr/local/bin/b5tou8
