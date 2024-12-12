from flask import Flask, render_template
app = Flask(__name__)
import pandas as pd


@app.route('/')
def home():
   dfs=[]
   dfs.append( pd.read_csv('AutBus2023.csv'))
   dfs.append(pd.read_csv('ConsBus2023.csv'))
   dfs.append(pd.read_csv('GHHBus2023.csv'))
   
   return render_template('buisness.html')
if __name__ == '__main__':
   app.run()