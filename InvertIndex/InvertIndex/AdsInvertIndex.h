#pragma once
#ifndef _ADSIRINDEX__
#define _ADSIRINDEX__

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <thread>

#include <Windows.h>
#include <direct.h>
#include <assert.h>

#include "AdsItem.h"
using namespace std;
#define PI 3.14159265

class AdsInvertIndex{
public:
	/* save invert index */
	unordered_map<string, vector<AdsItem*>> adsMap;
	/* constructor */
	AdsInvertIndex(vector<AdsItem>* adsPtr) : Weight(100, 10, 1), bidWeight(0.1, 0.2, 0.7){
		addAdsSet(*adsPtr);
		for (auto & iter : this->adsMap){
			auto & arr = iter.second;
			normalizeScore(arr);
			sort(arr.begin(), arr.end(),
				[](AdsItem* & const lhs, AdsItem* & const rhs)->bool{return lhs->score > rhs->score; });
		}
	}
	void addAdsSet(vector<AdsItem>& ads){
		using std::get;
		for (auto & adsItem : ads){
			//AdsItem adsItem(line);
			auto keyWordsTuple = adsItem.getKeyWordSet();
			vector<string>& bidKeywords = get<0>(keyWordsTuple);
			vector<string>& titleKeywords = get<1>(keyWordsTuple);
			vector<string>& textKeywords = get<2>(keyWordsTuple);

			createIndex_BidKeywords(bidKeywords, adsItem);
			createIndex_AdTitileKeywords(titleKeywords, adsItem);
			createIndex_AdTextKeywords(textKeywords, adsItem);
		}
	}
	/* forbid copy constructor and assignment */
	AdsInvertIndex(const AdsInvertIndex&) = delete;
	void operator = (const AdsInvertIndex&) = delete;
	/* search functions */
	vector<AdsItem*> search(string& key, int i){
		auto iter = this->adsMap.find(key);
		if (iter == this->adsMap.end())
			return{};
		else{
			auto ret = iter->second;
			//need to improve sort function
			std::sort(ret.begin(), ret.end(), [](AdsItem* & const lhs, AdsItem* & const rhs)->bool{return lhs->exactBid > rhs->exactBid; });
			if (ret.size() > i)
				ret.resize(i);
			return ret;
		}
	}

	/*normalize all scores*/
	void normalizeScore(vector<AdsItem*>& ads) {
		for (auto & adsItem : ads){
			//x = x * atan(pow(x, -0.5));
			adsItem->score = atan(adsItem->score) * 2 / PI;
			//adsItem->score = pow(sqrt(adsItem->score), -1);
		}
	}

private:
	/*!!!wating for adding different rules*/
	void createIndex_BidKeywords(vector<string>& bidKws, AdsItem &item){
		item.score = rateAdsItem(item, this->bidWeight);
		for (auto & x : bidKws){
			addToAdsMap(x, item);
		}
	}

	void createIndex_AdTitileKeywords(vector<string>& titleKws, AdsItem &item){
		for (auto & x : titleKws){
			//		item.score = rateAdsItem(item, this->Weight.fromTitle);
			addToAdsMap(x, item);
		}
	}

	void createIndex_AdTextKeywords(vector<string>& textKws, AdsItem &item){
		for (auto & x : textKws){
			//		item.score = rateAdsItem(item, this->Weight.fromText);
			addToAdsMap(x, item);
		}
	}
	void keyWordNormalizer(string& key){
		int len = key.length();
		int write = 0;
		for (int i = 0; i < key.size(); i++){
			if (key[i] == '?' || key[i] == '+'){
				len--;
			}
			else{
				key[write++] = key[i];
			}
		}
		key.resize(len);
	}
	void addToAdsMap(string& key, AdsItem& val){
		keyWordNormalizer(key);
		auto iter = this->adsMap.find(key);
		if (iter == this->adsMap.end())
			this->adsMap[key] = { &val };
		else
			this->adsMap[key].push_back(&val);
	}

	/* sort: top K */
	void SortAdsVector(string& key){
		auto & adsVec = this->adsMap[key];
	}

	/* different keywords weight for kws from different parts */
	struct KeywordsWeight{
		double fromBid;
		double fromTitle;
		double fromText;
		KeywordsWeight(double a, double b, double c) :fromBid(a), fromTitle(b), fromText(c){}
	}Weight;

	struct BitWeight{
		double exactBid;
		double phraseBid;
		double broadBid;
		BitWeight(double a, double b, double c) :exactBid(a), phraseBid(b), broadBid(c){}
	}bidWeight;

	/*rating function*/
	double rateAdsItem(AdsItem& item, BitWeight weight){
		double score = item.exactBid * weight.exactBid + item.phraseBid * weight.phraseBid + item.broadBid * weight.broadBid;
		return score;
	}

	/*sort's compare function implementation*/
	bool cmp(const AdsItem& lhs, const AdsItem& rhs){
		return true;
	}
public:
	void writeToFile(string& outputDir){
		if (outputDir[outputDir.size() - 1] != '/')
			outputDir += '/';
		DWORD attribute = GetFileAttributes(outputDir.c_str());
		if (attribute == INVALID_FILE_ATTRIBUTES){
			_mkdir(outputDir.c_str());
		}

		char s[100];
		string errfile = outputDir + "ErrorWords.csv";
		FILE *ferr = fopen(errfile.c_str(), "w");
		for (auto & pair : this->adsMap){
			string fname = outputDir + pair.first;
			FILE *fp = fopen(fname.c_str(), "w");
			if (fp == NULL){
				sprintf(s, "%s ,%d\n", pair.first.c_str(), pair.second.size());
				fwrite(s, sizeof(char), strlen(s), ferr);
			}
			else{
				assert(fp != NULL);
				for (auto ptr : pair.second){
					sprintf(s, "{\"ListingId\" : %s, \"AdId\" : %s, \"Score\" : \%.5f\}\n", ptr->listingId.c_str(), ptr->adId.c_str(), ptr->score);
					fwrite(s, sizeof(char), strlen(s), fp);
				}
				fclose(fp);
			}
		}
		fclose(ferr);
	}
};
#endif