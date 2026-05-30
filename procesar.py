import pandas as pd

# Load CSV
df = pd.read_csv('metricas.csv')
scalars = df[df['type'] == 'scalar']

def get_scalar(module, name):
    val = scalars[(scalars['module'] == module) & (scalars['name'] == name)]
    if not val.empty:
        return float(val.iloc[0]['value'])
    return 0.0

# Table metrics
avg_delay = get_scalar('Network.node[5].app', 'Average delay')
avg_hops = get_scalar('Network.node[5].app', 'Average hop count')
max_hops = get_scalar('Network.node[5].app', 'Max hop count')
hop_std = get_scalar('Network.node[5].app', 'Hop count standard deviation')

buffers_avg = scalars[scalars['name'] == 'Average buffer length']['value'].astype(float)
buffers_max = scalars[scalars['name'] == 'Max buffer length']['value'].astype(float)
utils = scalars[scalars['name'] == 'Link utilization (%)']['value'].astype(float)

buf_prom = buffers_avg.mean() if not buffers_avg.empty else 0.0
buf_max = buffers_max.max() if not buffers_max.empty else 0.0
buf_std = buffers_avg.std() if len(buffers_avg) > 1 else 0.0  # approximate

util_prom = (utils.mean()) if not utils.empty else 0.0
util_max = (utils.max()) if not utils.empty else 0.0
util_std = (utils.std()) if len(utils) > 1 else 0.0

# Source specific metrics
del_0 = get_scalar('Network.node[5].app', 'Average delay Src 0')
del_2 = get_scalar('Network.node[5].app', 'Average delay Src 2')
hop_0 = get_scalar('Network.node[5].app', 'Average hops Src 0')
hop_2 = get_scalar('Network.node[5].app', 'Average hops Src 2')

# Node 0 and Node 2 specific buffers/links
buf_0 = get_scalar('Network.node[0].lnk[0]', 'Max buffer length')
util_0 = get_scalar('Network.node[0].lnk[0]', 'Link utilization (%)')

buf_2 = get_scalar('Network.node[2].lnk[0]', 'Max buffer length')
util_2 = get_scalar('Network.node[2].lnk[0]', 'Link utilization (%)')

# Normalize values for graph (max scaled)
max_d = max(del_0, del_2) if max(del_0, del_2) > 0 else 1
max_h = max(hop_0, hop_2) if max(hop_0, hop_2) > 0 else 1
max_b = max(buf_0, buf_2) if max(buf_0, buf_2) > 0 else 1
max_u = max(util_0, util_2) if max(util_0, util_2) > 0 else 1

d0_n = del_0 / max_d
d2_n = del_2 / max_d
h0_n = hop_0 / max_h
h2_n = hop_2 / max_h
b0_n = buf_0 / max_b
b2_n = buf_2 / max_b
u0_n = util_0 / max_u
u2_n = util_2 / max_u

latex_code = f"""
\\begin{{table}}[!ht]
  \\centering
  \\caption{{Resultados del Caso~1 (baseline kickstarter).}}
  \\label{{tab:caso1}}
  \\coltablesetup
  \\resizebox{{\\columnwidth}}{{!}}{{%\n  \\begin{{tabular}}{{@{{}}lrrr@{{}}}}
    \\toprule
    \\textbf{{Métrica}} & \\textbf{{Prom.}} & \\textbf{{Máx.}} & \\textbf{{Desv.}} \\\\
    \\midrule
    Demora (s)        & {avg_delay:.2f} & 0.00 & 0.00 \\\\
    Saltos            & {avg_hops:.1f}  & {max_hops:.0f}    & {hop_std:.2f}  \\\\
    Ocupación buffer  & {buf_prom:.1f}  & {buf_max:.0f}    & {buf_std:.1f}  \\\\
    Util. enlace (\\%) & {util_prom:.1f}  & {util_max:.1f}  & {util_std:.1f}  \\\\
    \\bottomrule
  \\end{{tabular}}%
  }}
\\end{{table}}

\\begin{{figure}}[!t]
  \\centering
  \\begin{{tikzpicture}}
    \\begin{{axis}}[
      width=\\linewidth,
      height=4.2cm,
      ybar,
      bar width=8pt,
      ymin=0,
      symbolic x coords={{Demora,Saltos,Buffer,Enlace}},
      xtick=data,
      x tick label style={{font=\\scriptsize, rotate=25, anchor=east}},
      ylabel={{Valor norm.}},
      title={{\\scriptsize Comparación Caso 1 por Fuente}},
      legend style={{font=\\scriptsize, at={{(0.5,1.05)}}, anchor=south, legend columns=-1}},
    ]
      \\addplot coordinates {{(Demora,{d0_n:.2f}) (Saltos,{h0_n:.2f}) (Buffer,{b0_n:.2f}) (Enlace,{u0_n:.2f})}};
      \\addplot coordinates {{(Demora,{d2_n:.2f}) (Saltos,{h2_n:.2f}) (Buffer,{b2_n:.2f}) (Enlace,{u2_n:.2f})}};
      \\legend{{Fuente 0 (atravesando Nodo 0), Fuente 2 (atravesando Nodo 2)}}
    \\end{{axis}}
  \\end{{tikzpicture}}
  \\caption{{Métricas por fuente de origen para el Caso 1 (valores normalizados).}}
  \\label{{fig:caso1-demora}}
\\end{{figure}}
"""

print(latex_code)
